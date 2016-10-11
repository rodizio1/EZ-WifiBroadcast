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

#include "tx_lib.h"

int flagHelp = 0;


void
usage(void)
{
    printf(
        "tx (c)2015 befinitiv. Based on packetspammer by Andy Green. Licensed under GPL2\n"
        "\n"
        "Usage: tx [options] <interface>\n"
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
        "  cat /dev/zero | tx -b 8 -r 4 -f 1024 wlan0 (reads zeros from stdin and sends them out on wlan0)\n"
        "\n", 1024, MAX_USER_PACKET_LENGTH, FIFO_NAME);
    exit(1);
}

typedef struct {
	int seq_nr;
	int fd;
	int curr_pb;
	packet_buffer_t *pbl;
} fifo_t;

	

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


int
main(int argc, char *argv[])
{
	char szErrbuf[PCAP_ERRBUF_SIZE];
	int i;
	pcap_t *ppcap = NULL;
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
	int param_packet_length = 1024;
	int param_port = 0;
	int param_min_packet_length = 0;
	int param_fifo_count = 1;


	printf("Raw data transmitter (c) 2015 befinitiv  GPL2\n");

	while (1) {
		int nOptionIndex;
		static const struct option optiona[] = {
			{ "help", no_argument, &flagHelp, 1 },
			{ 0, 0, 0, 0 }
		};
		int c = getopt_long(argc, argv, "r:hf:p:b:m:s:x:",
			optiona, &nOptionIndex);

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

	if (optind >= argc)
		usage();

	
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


	// open the interface in pcap
	szErrbuf[0] = '\0';
	ppcap = pcap_open_live(argv[optind], 800, 1, 20, szErrbuf);
	if (ppcap == NULL) {
		fprintf(stderr, "Unable to open interface %s in pcap: %s\n",
		    argv[optind], szErrbuf);
		return (1);
	}


	pcap_setnonblock(ppcap, 0, szErrbuf);




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
                    pb_transmit_block(fifo[i].pbl, ppcap, &(fifo[i].seq_nr), i+param_port, param_packet_length, packet_transmit_buffer, packet_header_length, param_data_packets_per_block, param_fec_packets_per_block, param_transmission_count);
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
