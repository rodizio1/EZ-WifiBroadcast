/*   (c) 2015 befinitiv
 *
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/resource.h>
#include "fec.h"
#include "lib.h"
#include "wifibroadcast.h"

#define MAX_PACKET_LENGTH 4192
#define MAX_USER_PACKET_LENGTH 1450
#define MAX_DATA_OR_FEC_PACKETS_PER_BLOCK 32

#define FIFO_NAME "/tmp/fifo%d"
#define MAX_FIFOS 8



static u8 u8aRadiotapHeader[] = {
	0x00, 0x00, // <-- radiotap version
	0x0c, 0x00, // <- radiotap header length
	0x04, 0x80, 0x00, 0x00, // <-- radiotap present flags
	0x00, // datarate (will be overwritten later in packet_header_init)
	0x00,
	0x00, 0x00
};

static u8 u8aIeeeHeader_data_short[] = {
	0x08, 0xbf, 0x00, 0x00, // frame control field (2 bytes), duration (2 bytes)
	0xff, 0x00, // 1st byte of IEEE802.11 RA (mac) must be 0xff or something odd, otherwise strange things happen. second byte is the port (will be overwritten later)
};

static u8 u8aIeeeHeader_data[] = {
	0x08, 0xbf, 0x00, 0x00, // frame control field (2 bytes), duration (2 bytes)
	0xff, 0x00, 0x00, 0x00, 0x00, 0x00,// 1st byte of IEEE802.11 RA (mac) must be 0xff or something odd, otherwise strange things happen. second byte is the port (will be overwritten later)
	0x13, 0x22, 0x33, 0x44, 0x55, 0x66, // mac
	0x13, 0x22, 0x33, 0x44, 0x55, 0x66, // mac
	0x00, 0x00, // IEEE802.11 seqnum, (will be overwritten later by Atheros firmware/wifi chip)
};

static u8 u8aIeeeHeader_cts[] = {
	0xc4, 0x00, 0x00, 0x00, // frame control field (2 bytes), duration (2 bytes)
	0xff, 0x00, // 1st byte of IEEE802.11 RA (mac) must be 0xff, otherwise strange things happen. second byte is the port (will be overwritten later)
};

static u8 u8aIeeeHeader_rts[] = {
	0xb4, 0x00, 0x00, 0x00, // frame control field (2 bytes), duration (2 bytes)
	0xff, 0x00, // 1st byte of IEEE802.11 RA (mac) must be 0xff, otherwise strange things happen. second byte is the port (will be overwritten later)
};

static u8 u8aIeeeHeader_ack[] = {
	0xd4, 0x00, 0x00, 0x00, // frame control field (2 bytes), duration (2 bytes)
	0xff, 0x00, // 1st byte of IEEE802.11 RA (mac) must be 0xff, otherwise strange things happen. second byte is the port (will be overwritten later)
};

//static u8 u8aIeeeHeader_beacon_short[] = {
//	0x80, 0x00, 0x00, 0x00, // frame control field (2 bytes), duration (2 bytes)
//	0xff, 0x00, // 1st byte of IEEE802.11 RA (mac) must be 0xff, otherwise strange things happen. second byte is the port (will be overwritten later)
//};

static u8 u8aIeeeHeader_beacon[] = {
	0x80, 0x00, 0x00, 0x00, // frame control field (2 bytes), duration (2 bytes)
	0xff, 0x00, 0x00, 0x00, 0x00, 0x00,// 1st byte of IEEE802.11 RA (mac) must be 0xff or something odd, otherwise strange things happen. second byte is the port (will be overwritten later)
	0x13, 0x22, 0x33, 0x44, 0x55, 0x66, // mac
	0x13, 0x22, 0x33, 0x44, 0x55, 0x66, // mac
	0x00, 0x00, // IEEE802.11 seqnum, (will be overwritten later by Atheros firmware/wifi chip)
};






int flagHelp = 0;

void usage(void) {
    printf(
	"tx (c)2015 by befinitiv. Dirty mods by Rodizio. Based on packetspammer by Andy Green. Licensed under GPL2\n"
	"\n"
	"Usage: tx [options] <interfaces>\n"
	"\n"
	"Options:\n"
	"-b <count>  Number of data packets in a block (default 8). Needs to match with rx.\n"
	"-r <count>  Number of FEC packets per block (default 4). Needs to match with rx.\n"
	"-f <bytes>  Number of bytes per packet (default %d, max. %d). This is also the FEC block size. Needs to match with rx.\n"
	"-m <bytes>  Minimum number of bytes per frame (default: 0)\n"
	"-p <port>   Port number 0-255 (default 0)\n"
	"-t <type>   Frame type to send. 0 = DATA short, 1 = DATA standard, 2 = CTS, 3 = RTS, 4 = ACK, 5 = Beacon standard\n"
	"-d <rate>   Data rate to send frames with. Currently only supported with Ralink cards. Choose 6,12,18,24,36 Mbit\n"
	"-y <mode>   Transmission mode. 0 = send on all interfaces, 1 = send only on interface with best RSSI\n"
        "\n"
        "Example:\n"
        "  cat /dev/zero | tx -b 8 -r 4 -f 1024 -t 1 -y 0 wlan0 (reads zeros from stdin and sends them out on wlan0) as standard DATA frames\n"
        "\n", 1024, MAX_USER_PACKET_LENGTH);
    exit(1);
}


typedef struct {
	int seq_nr;
	int fd;
	int curr_pb;
	packet_buffer_t *pbl;
} fifo_t;


int packet_header_init(uint8_t *packet_header, int type, int rate, int port) {
	u8 *pu8 = packet_header;

	switch (rate) {
	    case 6:
		u8aRadiotapHeader[8]=0x0c;
		break;
	    case 12:
		u8aRadiotapHeader[8]=0x18;
		break;
	    case 18:
		u8aRadiotapHeader[8]=0x24;
		break;
	    case 24:
		u8aRadiotapHeader[8]=0x30;
		break;
	    case 36:
		u8aRadiotapHeader[8]=0x48;
		break;
	    default:
		fprintf(stderr, "ERROR: Wrong or no data rate specified (see -d parameter)\n");
		exit(1);
		break;
	}

	memcpy(packet_header, u8aRadiotapHeader, sizeof(u8aRadiotapHeader));
	pu8 += sizeof(u8aRadiotapHeader);

	switch (type) {
	    case 0: // short DATA frame (for Ralink video and telemetry)
		fprintf(stderr, "using short DATA frames\n");
		u8aIeeeHeader_data_short[5] = port;
		memcpy(pu8, u8aIeeeHeader_data_short, sizeof (u8aIeeeHeader_data_short));
		pu8 += sizeof (u8aIeeeHeader_data_short);
		break;
	    case 1: // standard DATA frame (for Atheros video with CTS protection)
		fprintf(stderr, "using standard DATA frames\n");
		u8aIeeeHeader_data[5] = port;
		memcpy(pu8, u8aIeeeHeader_data, sizeof (u8aIeeeHeader_data));
		pu8 += sizeof (u8aIeeeHeader_data);
		break;
	    case 2: // CTS frame (for Atheros telemetry)
		fprintf(stderr, "using CTS frames\n");
		u8aIeeeHeader_cts[5] = port;
		memcpy(pu8, u8aIeeeHeader_cts, sizeof (u8aIeeeHeader_cts));
		pu8 += sizeof (u8aIeeeHeader_cts);
		break;
	    case 3: // RTS frame
		fprintf(stderr, "using RTS frames\n");
		u8aIeeeHeader_rts[5] = port;
		memcpy(pu8, u8aIeeeHeader_rts, sizeof (u8aIeeeHeader_rts));
		pu8 += sizeof (u8aIeeeHeader_rts);
		break;
	    case 4: // ACK frame
		fprintf(stderr, "using ACK frames\n");
		u8aIeeeHeader_ack[5] = port;
		memcpy(pu8, u8aIeeeHeader_ack, sizeof (u8aIeeeHeader_ack));
		pu8 += sizeof (u8aIeeeHeader_ack);
		break;
	    case 5: // Beacon frame (for Atheros video without CTS protection)
		fprintf(stderr, "using Beacon frames\n");
		u8aIeeeHeader_beacon[5] = port;
		memcpy(pu8, u8aIeeeHeader_beacon, sizeof (u8aIeeeHeader_beacon));
		pu8 += sizeof (u8aIeeeHeader_beacon);
		break;

	    default:
		fprintf(stderr, "ERROR: Wrong or no frame type specified (see -t parameter)\n");
		exit(1);
		break;
	}

	//determine the length of the header
	return pu8 - packet_header;
}

void fifo_init(fifo_t *fifo, int fifo_count, int block_size) {
	int i;

	for(i=0; i<fifo_count; ++i) {
		int j;

		fifo[i].seq_nr = 0;
		fifo[i].fd = -1;
		fifo[i].curr_pb = 0;
		fifo[i].pbl = lib_alloc_packet_buffer_list(block_size, MAX_PACKET_LENGTH);

		//prepare the buffers with headers
		for(j=0; j<block_size; ++j) {
			fifo[i].pbl[j].len = 0;
		}
	}

}

void fifo_open(fifo_t *fifo, int fifo_count) {
	//old style STDIN input
	fifo[0].fd = STDIN_FILENO;
}


void fifo_create_select_set(fifo_t *fifo, int fifo_count, fd_set *fifo_set, int *max_fifo_fd) {
	FD_ZERO(fifo_set);
	FD_SET(fifo[0].fd, fifo_set);

	if(fifo[0].fd > *max_fifo_fd) {
		*max_fifo_fd = fifo[0].fd;
	}
}


void pb_transmit_packet(pcap_t *ppcap[4], int seq_nr, uint8_t *packet_transmit_buffer, int packet_header_len, const uint8_t *packet_data, int packet_length, int num_interfaces, int param_transmission_mode, int best_adapter) {
    int i = 0;

    //add header outside of FEC
    wifi_packet_header_t *wph = (wifi_packet_header_t*)(packet_transmit_buffer + packet_header_len);
    wph->sequence_number = seq_nr;

    //copy data
    memcpy(packet_transmit_buffer + packet_header_len + sizeof(wifi_packet_header_t), packet_data, packet_length);

    int plen = packet_length + packet_header_len + sizeof(wifi_packet_header_t);


    if (best_adapter == 5) {
	for(i=0; i<num_interfaces; ++i) {
    	    int r = pcap_inject(ppcap[i], packet_transmit_buffer, plen);
	    if (r != plen) {
    		pcap_perror(ppcap[i], "Trouble injecting packet");
    		exit(1);
	    }
	}
    } else {
    	int r = pcap_inject(ppcap[best_adapter], packet_transmit_buffer, plen);
	if (r != plen) {
    	    pcap_perror(ppcap[best_adapter], "Trouble injecting packet");
    	    exit(1);
	}
    }


}


void pb_transmit_block(packet_buffer_t *pbl, pcap_t *ppcap[4], int *seq_nr, int port, int packet_length, uint8_t *packet_transmit_buffer, int packet_header_len, int data_packets_per_block, int fec_packets_per_block, int num_interfaces, int param_transmission_mode, telemetry_data_t *td1) {
	int i;
	uint8_t *data_blocks[MAX_DATA_OR_FEC_PACKETS_PER_BLOCK];
	uint8_t fec_pool[MAX_DATA_OR_FEC_PACKETS_PER_BLOCK][MAX_USER_PACKET_LENGTH];
	uint8_t *fec_blocks[MAX_DATA_OR_FEC_PACKETS_PER_BLOCK];

	for(i=0; i<data_packets_per_block; ++i) {
		data_blocks[i] = pbl[i].data;
	}

	if(fec_packets_per_block) {
		for(i=0; i<fec_packets_per_block; ++i) {
			fec_blocks[i] = fec_pool[i];
		}

		fec_encode(packet_length, data_blocks, data_packets_per_block, (unsigned char **)fec_blocks, fec_packets_per_block);
	}

	if(td1->rx_status != NULL) {

	}

	uint8_t *pb = packet_transmit_buffer;
	pb += packet_header_len;

	//send data and FEC packets interleaved
	int di = 0;
	int fi = 0;
	int seq_nr_tmp = *seq_nr;
	while(di < data_packets_per_block || fi < fec_packets_per_block) {
	    int best_adapter = 0;
	    if(param_transmission_mode == 1) {
    		int i;
    		int ac = td1->rx_status->wifi_adapter_cnt;
    		int best_dbm = -1000;

    		// find out which card has best signal
    		for(i=0; i<ac; ++i) {
    		    if (best_dbm < td1->rx_status->adapter[i].current_signal_dbm) {
    			best_dbm = td1->rx_status->adapter[i].current_signal_dbm;
        		best_adapter = i;
    		    }
    		}
    		printf ("bestadapter: %d (%d dbm)\n",best_adapter, best_dbm);
	    } else {
		best_adapter = 5; // set to 5 to let transmit packet function know it shall transmit on all interfaces
	    }

	    if(di < data_packets_per_block) {
		pb_transmit_packet(ppcap, seq_nr_tmp, packet_transmit_buffer, packet_header_len, data_blocks[di], packet_length,num_interfaces, param_transmission_mode,best_adapter);
		seq_nr_tmp++;
		di++;
	    }

	    if(fi < fec_packets_per_block) {
		pb_transmit_packet(ppcap, seq_nr_tmp, packet_transmit_buffer, packet_header_len, fec_blocks[fi], packet_length,num_interfaces,param_transmission_mode,best_adapter);
		seq_nr_tmp++;
		fi++;
	    }	
	}

	*seq_nr += data_packets_per_block + fec_packets_per_block;

	//reset the length back
	for(i=0; i< data_packets_per_block; ++i) {
			pbl[i].len = 0;
	}

}


wifibroadcast_rx_status_t *telemetry_wbc_status_memory_open(void) {

    int fd = 0;
    int sharedmem = 0;

//    if (transmission_mode == 1) {
//	while(sharedmem == 0) {
    	    fd = shm_open("/wifibroadcast_rx_status_0", O_RDWR, S_IRUSR | S_IWUSR);
        	if(fd < 0) {
    //        	    fprintf(stderr, "Could not open wifibroadcast rx status - retrying ...\n");
        	} else {
            	    sharedmem = 1;
        	}
//        	usleep(150000);
//	}

//        if (ftruncate(fd, sizeof(wifibroadcast_rx_status_t)) == -1) {
//                perror("ftruncate");
//                exit(1);
//        }

        void *retval = mmap(NULL, sizeof(wifibroadcast_rx_status_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
//        if (retval == MAP_FAILED) {
//                perror("mmap");
//                exit(1);
//        }
//    }

    return (wifibroadcast_rx_status_t*)retval;
}

void telemetry_init(telemetry_data_t *td) {
    td->rx_status = telemetry_wbc_status_memory_open();
}



int main(int argc, char *argv[]) {

    setpriority(PRIO_PROCESS, 0, -10);

    char szErrbuf[PCAP_ERRBUF_SIZE];
    pcap_t *ppcap[4];
    char fBrokenSocket = 0;
    int pcnt = 0;
    uint8_t packet_transmit_buffer[MAX_PACKET_LENGTH];
    size_t packet_header_length = 0;
    fd_set fifo_set;
    int max_fifo_fd = -1;
    fifo_t fifo[MAX_FIFOS];

    int param_data_packets_per_block = 8;
    int param_fec_packets_per_block = 4;
    int param_packet_length = 1024;
    int param_port = 0;
    int param_min_packet_length = 0;
    int param_fifo_count = 1;
    int param_packet_type = 99;
    int param_data_rate = 99;
    int param_transmission_mode = 0;

    printf("Raw data transmitter (c) 2015 befinitiv. Dirty mods by Rodizio.  GPL2\n");

    while (1) {
	int nOptionIndex;
	static const struct option optiona[] = {
	    { "help", no_argument, &flagHelp, 1 },
	    { 0, 0, 0, 0 }
	};

	int c = getopt_long(argc, argv, "h:r:f:p:b:m:t:d:y:", optiona, &nOptionIndex);
	if (c == -1) break;
	switch (c) {
	    case 0: // long option
		break;

	    case 'h': // help
		usage();
		break;

	    case 'r': // retransmissions
		param_fec_packets_per_block = atoi(optarg);
		break;

	    case 'f': // MTU
		param_packet_length = atoi(optarg);
		break;

	    case 'p': //port
		param_port = atoi(optarg);
		break;

	    case 'b': //retransmission block size
		param_data_packets_per_block = atoi(optarg);
		break;

	    case 'm': //minimum packet length
		param_min_packet_length = atoi(optarg);
		break;

	    case 't': // packet type
		param_packet_type = atoi(optarg);
		break;

	    case 'd': // data rate
		param_data_rate = atoi(optarg);
		break;

	    case 'y': // transmission mode
		param_transmission_mode = atoi(optarg);
		break;

	    default:
		fprintf(stderr, "unknown switch %c\n", c);
		usage();
		break;

	}
    }

    if (optind >= argc) usage();

    if(param_packet_length > MAX_USER_PACKET_LENGTH) {
	fprintf(stderr, "ERROR; Packet length is limited to %d bytes (you requested %d bytes)\n", MAX_USER_PACKET_LENGTH, param_packet_length);
	return (1);
    }

    if(param_min_packet_length > param_packet_length) {
	fprintf(stderr, "ERROR; Minimum packet length is higher than maximum packet length (%d > %d)\n", param_min_packet_length, param_packet_length);
	return (1);
    }

    if(param_data_packets_per_block > MAX_DATA_OR_FEC_PACKETS_PER_BLOCK || param_fec_packets_per_block > MAX_DATA_OR_FEC_PACKETS_PER_BLOCK) {
	fprintf(stderr, "ERROR: Data and FEC packets per block are limited to %d (you requested %d data, %d FEC)\n", MAX_DATA_OR_FEC_PACKETS_PER_BLOCK, param_data_packets_per_block, param_fec_packets_per_block);
	return (1);
    }

    packet_header_length = packet_header_init(packet_transmit_buffer, param_packet_type, param_data_rate, param_port);
    fifo_init(fifo, param_fifo_count, param_data_packets_per_block);
    fifo_open(fifo, param_fifo_count);
    fifo_create_select_set(fifo, param_fifo_count, &fifo_set, &max_fifo_fd);

    //initialize forward error correction
    fec_init();


    // initialize telemetry shared mem for rssi based transmission (-y 1)
    telemetry_data_t td;
    telemetry_init(&td);

    int x = optind;
    int num_interfaces = 0;

//    fprintf(stderr,"optind:%i\n",optind);

    while(x < argc && num_interfaces < 4) {
	// open the interface in pcap, no promiscous mode, snaplen 100
	szErrbuf[0] = '\0';
	ppcap[num_interfaces] = pcap_open_live(argv[x], 100, 0, 20, szErrbuf);
	if (ppcap[num_interfaces] == NULL) {
	    fprintf(stderr, "Unable to open interface %s in pcap: %s\n",
	    argv[x], szErrbuf);
	    return (1);
	}
	pcap_setnonblock(ppcap[num_interfaces], 0, szErrbuf);

        ++num_interfaces;
        ++x;
        usleep(20000); // wait a bit between configuring interfaces to reduce Atheros and Pi USB flakiness
    }

    while (!fBrokenSocket) {
	fd_set rdfs;
	int ret;

	rdfs = fifo_set;

	//wait for new data on the fifos
	ret = select(max_fifo_fd + 1, &rdfs, NULL, NULL, NULL);

	if(ret < 0) {
	    perror("select");
	    return (1);
	}

	if(!FD_ISSET(fifo[0].fd, &rdfs)) {
	    continue;
	}

	ret--;

	packet_buffer_t *pb = fifo[0].pbl + fifo[0].curr_pb;

	//if the buffer is fresh we add a payload header
	if(pb->len == 0) {
	    pb->len += sizeof(payload_header_t); //make space for a length field (will be filled later)
	}

	//read the data
	int inl = read(fifo[0].fd, pb->data + pb->len, param_packet_length - pb->len);
	if(inl < 0 || inl > param_packet_length-pb->len) {
	    perror("reading stdin");
	    return 1;
	}

	if(inl == 0) {
	    //EOF
	    fprintf(stderr, "Warning: Lost connection to stdin. Please make sure that a data source is connected\n");
	    usleep(1e5);
	    continue;
	}

	pb->len += inl;

	//check if this packet is finished
	if(pb->len >= param_min_packet_length) {
	    payload_header_t *ph = (payload_header_t*)pb->data;
	    // write the length into the packet. this is needed since with fec we cannot use the wifi packet lentgh anymore.
	    // We could also set the user payload to a fixed size but this would introduce additional latency since tx would need to wait until that amount of data has been received
	    ph->data_length = pb->len - sizeof(payload_header_t);
	    pcnt++;
	    //check if this block is finished
	    if(fifo[0].curr_pb == param_data_packets_per_block-1) {
		pb_transmit_block(fifo[0].pbl, ppcap, &(fifo[0].seq_nr), param_port, param_packet_length, packet_transmit_buffer, packet_header_length, param_data_packets_per_block, param_fec_packets_per_block, num_interfaces, param_transmission_mode, &td);
		fifo[0].curr_pb = 0;
	    } else {
		fifo[0].curr_pb++;
	    }
	}

	if(pcnt % 128 == 0) {
	    printf("%d data packets sent\r", pcnt);
	}

    }

    printf("Broken socket\n");
    return (0);
}
