// (c)2015 befinitiv

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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <time.h>

#include "fec.h"

#include "lib.h"
#include "wifibroadcast.h"



#define MAX_PACKET_LENGTH 4192
#define MAX_USER_PACKET_LENGTH 1450
#define MAX_DATA_OR_FEC_PACKETS_PER_BLOCK 32

#define FIFO_NAME "/tmp/fifo%d"
#define MAX_FIFOS 8


/* this is the template radiotap header we send packets out with */


static const u8 u8aRadiotapHeader[] = {

    0x00, 0x00, // <-- radiotap version
    0x0c, 0x00, // <- radiotap header lengt
    0x04, 0x80, 0x00, 0x00, // <-- radiotap present flags
//    0x24, // datarate 18Mbit
//	0x30, // datarate 24Mbit
//	0x48, // datarate 36Mbit
	0x60, // datarate 48Mbit
    0x0, 
    0x18, 0x00 
};


/* Penumbra IEEE80211 header */

//the last byte of the mac address is recycled as a port number
#define SRC_MAC_LASTBYTE 15
#define DST_MAC_LASTBYTE 21

static u8 u8aIeeeHeader[] = {
	0x08, 0x01, 0x00, 0x00,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x13, 0x22, 0x33, 0x44, 0x55, 0x66,
	0x13, 0x22, 0x33, 0x44, 0x55, 0x66,
	0x10, 0x86,
};



int flagHelp = 0;



void
usage(void)
{
    printf(
        "tx_duplicate (c)2015 befinitiv. Based on packetspammer by Andy Green. Dirty mods by Rodizio.  Licensed under GPL2\n"
        "\n"
        "Modified version to send out packets duplicated on wlan0 and wlan1\n"
        "\n"
        "Usage: tx_duplicate [options]\n"
        "\n"
        "Options:\n"
        "-b <count>  Number of data packets in a block (default 8). Needs to match with rx.\n"
        "-r <count>  Number of FEC packets per block (default 4). Needs to match with rx.\n"
        "-f <bytes>  Number of bytes per packet (default %d, max. %d). This is also the FEC block size. Needs to match with rx.\n"
        "-m <bytes>  Minimum number of bytes per frame (default: 0)\n"
        "-p <port>   Port number 0-255 (default 0)\n"
        "-s <stream> If <stream> is > 1 then the parameter changes \"tx\" input from stdin to named FIFOs. Each fifo transports\n"
        "            a stream over a different port (starting at -p port and incrementing). FIFO names are \"%s\". (default 1)\n"
        "\n"
        "Example:\n"
        "  cat /dev/zero | tx -b 8 -r 4 -f 1024 (reads zeros from stdin and sends them out duplicated on wlan0 and wlan1)\n"
        "\n", 1024, MAX_USER_PACKET_LENGTH, FIFO_NAME);
    exit(1);
}

void set_port_no(uint8_t *pu, uint8_t port) {
	//dirty hack: the last byte of the mac address is the port number. this makes it easy to filter out specific ports via wireshark
	pu[sizeof(u8aRadiotapHeader) + SRC_MAC_LASTBYTE] = port;
	pu[sizeof(u8aRadiotapHeader) + DST_MAC_LASTBYTE] = port;
}


typedef struct {
	int seq_nr;
	int fd;
	int curr_pb;
	packet_buffer_t *pbl;
} fifo_t;

	

int packet_header_init(uint8_t *packet_header) {
			u8 *pu8 = packet_header;
			memcpy(packet_header, u8aRadiotapHeader, sizeof(u8aRadiotapHeader));
			pu8 += sizeof(u8aRadiotapHeader);
			memcpy(pu8, u8aIeeeHeader, sizeof (u8aIeeeHeader));
			pu8 += sizeof (u8aIeeeHeader);

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
	int i;
	if(fifo_count > 1) {
		//new FIFO style
		
		//first, create all required fifos
		for(i=0; i<fifo_count; ++i) {
			char fn[256];
			sprintf(fn, FIFO_NAME, i);
			
			unlink(fn);
			if(mkfifo(fn, 0666) != 0) {
				fprintf(stderr, "Error creating FIFO \"%s\"\n", fn);
				exit(1);
			}
		}
		
		//second: wait for the data sources to connect
		for(i=0; i<fifo_count; ++i) {
			char fn[256];
			sprintf(fn, FIFO_NAME, i);
			
			printf("Waiting for \"%s\" being opened from the data source... \n", fn);			
			if((fifo[i].fd = open(fn, O_RDONLY)) < 0) {
				fprintf(stderr, "Error opening FIFO \"%s\"\n", fn);
				exit(1);
			}
			printf("OK\n");
		}
	}
	else {
		//old style STDIN input
		fifo[0].fd = STDIN_FILENO;
	}
}


void fifo_create_select_set(fifo_t *fifo, int fifo_count, fd_set *fifo_set, int *max_fifo_fd) {
	int i;

	FD_ZERO(fifo_set);
	
	for(i=0; i<fifo_count; ++i) {
		FD_SET(fifo[i].fd, fifo_set);

		if(fifo[i].fd > *max_fifo_fd) {
			*max_fifo_fd = fifo[i].fd;
		}
	}
}


void pb_transmit_packet(pcap_t *ppcap1, pcap_t *ppcap2, int seq_nr, uint8_t *packet_transmit_buffer, int packet_header_len, const uint8_t *packet_data, int packet_length) {
	//add header outside of FEC
	wifi_packet_header_t *wph = (wifi_packet_header_t*)(packet_transmit_buffer + packet_header_len);
	wph->sequence_number = seq_nr;

	//copy data
	memcpy(packet_transmit_buffer + packet_header_len + sizeof(wifi_packet_header_t), packet_data, packet_length);

	int plen = packet_length + packet_header_len + sizeof(wifi_packet_header_t);

	int r1 = pcap_inject(ppcap1, packet_transmit_buffer, plen);
	if (r1 != plen) {
		pcap_perror(ppcap1, "Trouble injecting packet on wlan0");
		exit(1);
	}

	int r2 = pcap_inject(ppcap2, packet_transmit_buffer, plen);
	if (r2 != plen) {
		pcap_perror(ppcap2, "Trouble injecting packet on wlan1");
		exit(1);
	}
}




void pb_transmit_block(packet_buffer_t *pbl, pcap_t *ppcap1, pcap_t *ppcap2, int *seq_nr, int port, int packet_length, uint8_t *packet_transmit_buffer, int packet_header_len, int data_packets_per_block, int fec_packets_per_block, int transmission_count) {
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

	uint8_t *pb = packet_transmit_buffer;
	set_port_no(pb, port);
	pb += packet_header_len;


	int x;
	for(x=0; x<transmission_count; ++x) {
		//send data and FEC packets interleaved
		int di = 0; 
		int fi = 0;
		int seq_nr_tmp = *seq_nr;
		while(di < data_packets_per_block || fi < fec_packets_per_block) {
			if(di < data_packets_per_block) {
				pb_transmit_packet(ppcap1, ppcap2, seq_nr_tmp, packet_transmit_buffer, packet_header_len, data_blocks[di], packet_length);
				seq_nr_tmp++;
				di++;
			}

			if(fi < fec_packets_per_block) {
				pb_transmit_packet(ppcap1, ppcap2, seq_nr_tmp, packet_transmit_buffer, packet_header_len, fec_blocks[fi], packet_length);
				seq_nr_tmp++;
				fi++;
			}	
		}
	}

	*seq_nr += data_packets_per_block + fec_packets_per_block;



	//reset the length back
	for(i=0; i< data_packets_per_block; ++i) {
			pbl[i].len = 0;
	}

}


int main(int argc, char *argv[]) {
	char szErrbuf1[PCAP_ERRBUF_SIZE];
	char szErrbuf2[PCAP_ERRBUF_SIZE];
	int i;
	pcap_t *ppcap1 = NULL;
	pcap_t *ppcap2 = NULL;
	char fBrokenSocket = 0;
	int pcnt = 0;
	time_t start_time;
	uint8_t packet_transmit_buffer[MAX_PACKET_LENGTH];
	size_t packet_header_length = 0;
	fd_set fifo_set;
	int max_fifo_fd = -1;
	fifo_t fifo[MAX_FIFOS];

	int param_transmission_count = 1;
	int param_data_packets_per_block = 8;
	int param_fec_packets_per_block = 4;
	int param_packet_length = MAX_USER_PACKET_LENGTH;
	int param_port = 0;
	int param_min_packet_length = 0;
	int param_fifo_count = 1;



	printf("tx_duplicate Raw data transmitter (c) 2015 befinitiv. Dirty mods by Rodizio.  GPL2\n");

	while (1) {
		int nOptionIndex;
		static const struct option optiona[] = {{ "help", no_argument, &flagHelp, 1 }, { 0, 0, 0, 0 }};
		int c = getopt_long(argc, argv, "r:hf:p:b:m:s:x:", optiona, &nOptionIndex);

		if (c == -1)
			break;
		switch (c) {
		case 0: // long option
			break;
		case 'h': // help
			usage();
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
		case 'm'://minimum packet length
			param_min_packet_length = atoi(optarg);
			break;
		case 's': //how many streams (fifos) do we have in parallel
			param_fifo_count = atoi(optarg);
			break;
		case 'x': //how often is a block transmitted
			param_transmission_count = atoi(optarg);
			break;
		default:
			fprintf(stderr, "unknown switch %c\n", c);
			usage();
			break;
		}
	}

	if (optind >= argc) usage();


	if(param_packet_length > MAX_USER_PACKET_LENGTH) {
		fprintf(stderr, "Packet length is limited to %d bytes (you requested %d bytes)\n", MAX_USER_PACKET_LENGTH, param_packet_length);
		return (1);
	}

	if(param_min_packet_length > param_packet_length) {
		fprintf(stderr, "Your minimum packet length is higher that your maximum packet length (%d > %d)\n", param_min_packet_length, param_packet_length);
		return (1);
	}

	if(param_fifo_count > MAX_FIFOS) {
		fprintf(stderr, "The maximum number of streams (FIFOS) is %d (you requested %d)\n", MAX_FIFOS, param_fifo_count);
		return (1);
	}

	if(param_data_packets_per_block > MAX_DATA_OR_FEC_PACKETS_PER_BLOCK || param_fec_packets_per_block > MAX_DATA_OR_FEC_PACKETS_PER_BLOCK) {
		fprintf(stderr, "Data and FEC packets per block are limited to %d (you requested %d data, %d FEC)\n", MAX_DATA_OR_FEC_PACKETS_PER_BLOCK, param_data_packets_per_block, param_fec_packets_per_block);
		return (1);
	}


	packet_header_length = packet_header_init(packet_transmit_buffer);
	fifo_init(fifo, param_fifo_count, param_data_packets_per_block);
	fifo_open(fifo, param_fifo_count);
	fifo_create_select_set(fifo, param_fifo_count, &fifo_set, &max_fifo_fd);


	//initialize forward error correction
	fec_init();


	// open the interfaces in pcap
	szErrbuf1[0] = '\0';
	szErrbuf2[0] = '\0';

	ppcap1 = pcap_open_live("wlan0", 800, 1, 20, szErrbuf1);
	ppcap2 = pcap_open_live("wlan1", 800, 1, 20, szErrbuf2);
	if (ppcap1 == NULL) {
		fprintf(stderr, "Unable to open interface wlan0 in pcap: %s\n", szErrbuf1);
		return (1);
	}

	if (ppcap2 == NULL) {
		fprintf(stderr, "Unable to open interface wlan1 in pcap: %s\n", szErrbuf2);
		return (1);
	}


	pcap_setnonblock(ppcap1, 0, szErrbuf1);
	pcap_setnonblock(ppcap2, 0, szErrbuf2);




	start_time = time(NULL);
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

		//cycle through all fifos and look for new data
		for(i=0; i<param_fifo_count && ret; ++i) {
			if(!FD_ISSET(fifo[i].fd, &rdfs)) {
				continue;
			}

			ret--;

			packet_buffer_t *pb = fifo[i].pbl + fifo[i].curr_pb;
			
			//if the buffer is fresh we add a payload header
			if(pb->len == 0) {
				pb->len += sizeof(payload_header_t); //make space for a length field (will be filled later)
			}

			//read the data
			int inl = read(fifo[i].fd, pb->data + pb->len, param_packet_length - pb->len);
			if(inl < 0 || inl > param_packet_length-pb->len){
				perror("reading stdin");
				return 1;
			}

			if(inl == 0) {
				//EOF
				fprintf(stderr, "Warning: Lost connection to fifo %d. Please make sure that a data source is connected\n", i);
				usleep(1e5);
				continue;
			}

			pb->len += inl;
			
			//check if this packet is finished
			if(pb->len >= param_min_packet_length) {
				payload_header_t *ph = (payload_header_t*)pb->data;
				ph->data_length = pb->len - sizeof(payload_header_t); //write the length into the packet. this is needed since with fec we cannot use the wifi packet lentgh anymore. We could also set the user payload to a fixed size but this would introduce additional latency since tx would need to wait until that amount of data has been received
				pcnt++;
				//check if this block is finished
				if(fifo[i].curr_pb == param_data_packets_per_block-1) {
					pb_transmit_block(fifo[i].pbl, ppcap1, ppcap2, &(fifo[i].seq_nr), i+param_port, param_packet_length, packet_transmit_buffer, packet_header_length, param_data_packets_per_block, param_fec_packets_per_block, param_transmission_count);
					fifo[i].curr_pb = 0;
				}
				else {
					fifo[i].curr_pb++;
				}
			}
		}

		if(pcnt % 128 == 0) {
			printf("%d data packets sent (interface rate: %.3f)\n", pcnt, 1.0 * pcnt / param_data_packets_per_block * (param_data_packets_per_block + param_fec_packets_per_block) / (time(NULL) - start_time));
		}
	}

	printf("Broken socket\n");

	return (0);
}
