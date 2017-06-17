// rssirx by Rodizio. Based on wifibroadcast rx by Befinitiv. Licensed under GPL2
/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; version 2.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "lib.h"
#include "wifibroadcast.h"
#include "radiotap.h"
#include <time.h>
#include <sys/resource.h>


// this is where we store a summary of the information from the radiotap header
typedef struct  {
	int m_nChannel;
	int m_nChannelFlags;
	int m_nRate;
	int m_nAntenna;
	int m_nRadiotapFlags;
} __attribute__((packed)) PENUMBRA_RADIOTAP_DATA;


int flagHelp = 0;
int param_baudrate = 115200;
int param_packet_type = 1;
int param_serialport = 0;


wifibroadcast_rx_status_t *rx_status = NULL;

void
usage(void)
{
	printf(
	    "rssirx by Rodizio. Based on wifibroadcast rx by Befinitiv. Licensed under GPL2\n"
	    "\n"
	    "Usage: rssirx [options] <interfaces>\n\nOptions\n"
	    "-t <type>       Packet type to receive. 0 = DATA short, 1 = DATA standard, 2 = CTS\n"
	    "\n"
	    "Example:\n"
	    "  rcrx -t 1 wlan0 (receive standard DATA frames on wlan0 and send payload to serialport)\n"
	    "\n");
	exit(1);
}

typedef struct {
	pcap_t *ppcap;
	int selectable_fd;
	int n80211HeaderLength;
} monitor_interface_t;



void open_and_configure_interface(const char *name, monitor_interface_t *interface) {
	struct bpf_program bpfprogram;
	char szProgram[512];
	char szErrbuf[PCAP_ERRBUF_SIZE];

	// open the interface in pcap
	szErrbuf[0] = '\0';

	interface->ppcap = pcap_open_live(name, 1600, 0, -1, szErrbuf);
	if (interface->ppcap == NULL) {
		fprintf(stderr, "Unable to open %s: %s\n", name, szErrbuf);
		exit(1);
	}
	
	if(pcap_setnonblock(interface->ppcap, 1, szErrbuf) < 0) {
		fprintf(stderr, "Error setting %s to nonblocking mode: %s\n", name, szErrbuf);
	}

	int nLinkEncap = pcap_datalink(interface->ppcap);

	if (nLinkEncap == DLT_IEEE802_11_RADIO) {
		interface->n80211HeaderLength = 0x18; // 24 bytes
		sprintf(szProgram, "ether[0x00:2] == 0x08bf && ether[0x04:2] == 0xff08"); // match on frametype, 1st byte of mac (ff) and portnumber (8 for rssi)
	} else {
		fprintf(stderr, "ERROR: unknown encapsulation on %s! check if monitor mode is supported and enabled\n", name);
		exit(1);
	}

	if (pcap_compile(interface->ppcap, &bpfprogram, szProgram, 1, 0) == -1) {
		puts(szProgram);
		puts(pcap_geterr(interface->ppcap));
		exit(1);
	} else {
		if (pcap_setfilter(interface->ppcap, &bpfprogram) == -1) {
			fprintf(stderr, "%s\n", szProgram);
			fprintf(stderr, "%s\n", pcap_geterr(interface->ppcap));
		} else {
		}
		pcap_freecode(&bpfprogram);
	}

	interface->selectable_fd = pcap_get_selectable_fd(interface->ppcap);
}



uint8_t process_packet(monitor_interface_t *interface, int adapter_no) {
		struct pcap_pkthdr * ppcapPacketHeader = NULL;
		struct ieee80211_radiotap_iterator rti;
		PENUMBRA_RADIOTAP_DATA prd;
		u8 payloadBuffer[100];
		u8 *pu8Payload = payloadBuffer;
		int bytes;
		int n;
		int retval;
		int u16HeaderLen;

		struct payloaddata_s {
		    int signal;
		    int lostpackets;
		}  __attribute__ ((__packed__));

		struct payloaddata_s payloaddata;

		// receive
		retval = pcap_next_ex(interface->ppcap, &ppcapPacketHeader,
		    (const u_char**)&pu8Payload);

		if (retval < 0) {
			if (strcmp("The interface went down",pcap_geterr(interface->ppcap)) == 0) {
			    fprintf(stderr, "rx: The interface went down\n");
			    exit(9);
			} else {
			    fprintf(stderr, "rx: %s\n", pcap_geterr(interface->ppcap));
			    exit(2);
			}
		}

//			printf("inside process_packet\n");

		if (retval != 1)
//			exit(1);
			return 0;
//			printf("inside process_packet after retval check\n");

		// fetch radiotap header length from radiotap header (seems to be 36 for Atheros and 18 for Ralink)
		u16HeaderLen = (pu8Payload[2] + (pu8Payload[3] << 8));
//		fprintf(stderr, "u16headerlen: %d\n", u16HeaderLen);

//		fprintf(stderr, "ppcapPacketHeader->len: %d\n", ppcapPacketHeader->len);
		if (ppcapPacketHeader->len < (u16HeaderLen + interface->n80211HeaderLength)) exit(1);

		bytes = ppcapPacketHeader->len - (u16HeaderLen + interface->n80211HeaderLength);
//		fprintf(stderr, "bytes: %d\n", bytes);
//		if (bytes < 0) return 257;

//		if (bytes < 0) exit(1);
		if (bytes < 0) return(0);
//			printf("inside process_packet after bytes check\n");

		if (ieee80211_radiotap_iterator_init(&rti, (struct ieee80211_radiotap_header *)pu8Payload, ppcapPacketHeader->len) < 0) exit(1);

		while ((n = ieee80211_radiotap_iterator_next(&rti)) == 0) {
			switch (rti.this_arg_index) {
		    // we don't use these radiotap infos right now, disabled
		    /*
			case IEEE80211_RADIOTAP_RATE:
				prd.m_nRate = (*rti.this_arg);
				break;

			case IEEE80211_RADIOTAP_CHANNEL:
				prd.m_nChannel =
				    le16_to_cpu(*((u16 *)rti.this_arg));
				prd.m_nChannelFlags =
				    le16_to_cpu(*((u16 *)(rti.this_arg + 2)));
				break;

			case IEEE80211_RADIOTAP_ANTENNA:
				prd.m_nAntenna = (*rti.this_arg) + 1;
				break;
		    */
			case IEEE80211_RADIOTAP_FLAGS:
				prd.m_nRadiotapFlags = *rti.this_arg;
				break;
//			case IEEE80211_RADIOTAP_DBM_ANTSIGNAL:
//				rx_status->adapter[adapter_no].current_signal_dbm = (int8_t)(*rti.this_arg);
//				break;

			}
		}
		pu8Payload += u16HeaderLen + interface->n80211HeaderLength;

		memcpy(&payloaddata,pu8Payload,8); // copy payloaddata (signal and lost packets count) to struct

//		printf ("signal:%d\n",payloaddata.signal);
//		printf ("lostpackets:%d\n",payloaddata.lostpackets);
		rx_status->adapter[0].current_signal_dbm = payloaddata.signal;
		rx_status->lost_packet_cnt = payloaddata.lostpackets;
//		write(STDOUT_FILENO, pu8Payload, 8);
		return(0);
}

void status_memory_init(wifibroadcast_rx_status_t *s) {
	s->received_block_cnt = 0;
	s->damaged_block_cnt = 0;
	s->received_packet_cnt = 0;
	s->lost_packet_cnt = 0;
	s->tx_restart_cnt = 0;
	s->wifi_adapter_cnt = 0;

	int i;
	for(i=0; i<8; ++i) {
		s->adapter[i].received_packet_cnt = 0;
		s->adapter[i].wrong_crc_cnt = 0;
		s->adapter[i].current_signal_dbm = 0;
	}
}


wifibroadcast_rx_status_t *status_memory_open(void) {
	char buf[128];
	int fd;
	
	sprintf(buf, "/wifibroadcast_rx_status_9");
	fd = shm_open(buf, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);

	if(fd < 0) {
		perror("shm_open");
		exit(1);
	}

	if (ftruncate(fd, sizeof(wifibroadcast_rx_status_t)) == -1) {
		perror("ftruncate");
		exit(1);
	}

	void *retval = mmap(NULL, sizeof(wifibroadcast_rx_status_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (retval == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}
	
	wifibroadcast_rx_status_t *tretval = (wifibroadcast_rx_status_t*)retval;
	status_memory_init(tretval);
	
	return tretval;

}

int main(int argc, char *argv[])
{

	printf("RSSI RX started\n");
	setpriority(PRIO_PROCESS, 0, -10);

	monitor_interface_t interfaces[8];
	int num_interfaces = 0;
	int i;
	int result = 0;

	while (1) {
		int nOptionIndex;
		static const struct option optiona[] = {
			{ "help", no_argument, &flagHelp, 1 },
			{ 0, 0, 0, 0 }
		};
		int c = getopt_long(argc, argv, "h:t:",
			optiona, &nOptionIndex);

		if (c == -1)
			break;
		switch (c) {
		case 0: // long option
			break;
		case 'h': // help
			usage();
		case 't': // packet type
			param_packet_type = atoi(optarg);
			break;
		default:
			fprintf(stderr, "unknown switch %c\n", c);
			usage();
		}
	}

	if (optind >= argc)
		usage();
	
	int x = optind;
	while(x < argc && num_interfaces < 8) {
		open_and_configure_interface(argv[x], interfaces + num_interfaces);
		++num_interfaces;
		++x;
		usleep(10000); // wait a bit between configuring interfaces to reduce Atheros and Pi USB flakiness
	}

	rx_status = status_memory_open();
	rx_status->wifi_adapter_cnt = num_interfaces;

	for(;;)
	{
		fd_set readset;
		struct timeval to;
//		printf("inside for loop\n");

		to.tv_sec = 0;
		to.tv_usec = 1e5;
	
		FD_ZERO(&readset);
		for(i=0; i<num_interfaces; ++i)
			FD_SET(interfaces[i].selectable_fd, &readset);

		int n = select(30, &readset, NULL, NULL, &to);

//		printf("inside for loop2\n");

		for(i=0; i<num_interfaces; ++i) {
//			printf("inside inner for loop\n");
			if(n == 0) {
//			    printf("n == 0\n");
		    	    //break;
			}
			if(FD_ISSET(interfaces[i].selectable_fd, &readset)) {
			    result = process_packet(interfaces + i, i);
			}
		}
	}

	return (0);
}
