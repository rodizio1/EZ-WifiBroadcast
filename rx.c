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

#include "rx_lib.h"

int param_port = 0;
int param_data_packets_per_block = 8;
int param_fec_packets_per_block = 4;
int param_block_buffers = 1;
int param_packet_length = MAX_USER_PACKET_LENGTH;
int flagHelp = 0;
wifibroadcast_rx_status_t *rx_status = NULL;

void usage(void)
{
	printf(
	    "(c)2015 befinitiv. Based on packetspammer by Andy Green.  Licensed under GPL2\n"
	    "\n"
	    "Usage: rx [options] <interfaces>\n\nOptions\n"
	    "-p <port>   Port number 0-255 (default 0)\n"
	    "-b <count>  Number of data packets in a block (default 8). Needs to match with tx.\n"
	    "-r <count>  Number of FEC packets per block (default 4). Needs to match with tx.\n"
	    "-f <bytes>  Number of bytes per packet (default %d. max %d). This is also the FEC block size. Needs to match with tx\n"
	    "-d <blocks> Number of transmissions blocks that are buffered (default 1). This is needed in case of diversity if one\n"
	    "            adapter delivers data faster than the other. Note that this increases latency\n"
	    "\n"
	    "Example:\n"
	    "  rx -b 8 -r 4 -f 1024 wlan0 | cat /dev/null  (receive raw packets on wlan0 and throw away payload to /dev/null)\n"
	    "\n", MAX_USER_PACKET_LENGTH, MAX_USER_PACKET_LENGTH);
	exit(1);
}


int
main(int argc, char *argv[])
{
	monitor_interface_t interfaces[MAX_PENUMBRA_INTERFACES];
	int num_interfaces = 0;
	int i;
	
    block_buffer_t *block_buffer_list;


	while (1) {
		int nOptionIndex;
		static const struct option optiona[] = {
			{ "help", no_argument, &flagHelp, 1 },
			{ 0, 0, 0, 0 }
		};
		int c = getopt_long(argc, argv, "hp:b:d:r:f:",
			optiona, &nOptionIndex);

		if (c == -1)
			break;
		switch (c) {
		case 0: // long option
			break;

		case 'h': // help
			usage();

		case 'p': //port
			param_port = atoi(optarg);
			break;
		
		case 'b': 
			param_data_packets_per_block = atoi(optarg);
			break;

		case 'r': 
			param_fec_packets_per_block = atoi(optarg);
			break;
		
		case 'd':
            param_block_buffers = atoi(optarg);
			break;
		
		case 'f': // MTU
			param_packet_length = atoi(optarg);
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
		printf("Packet length is limited to %d bytes (you requested %d bytes)\n", MAX_USER_PACKET_LENGTH, param_packet_length);
		return (1);
	}


	fec_init();

	int x = optind;
	while(x < argc && num_interfaces < MAX_PENUMBRA_INTERFACES) {
		open_and_configure_interface(argv[x], param_port, interfaces + num_interfaces);
		++num_interfaces;
		++x;
	}


    //block buffers contain both the block_num as well as packet buffers for a block.
    block_buffer_list = malloc(sizeof(block_buffer_t) * param_block_buffers);
    for(i=0; i<param_block_buffers; ++i)
	{
        block_buffer_list[i].block_num = -1;
        block_buffer_list[i].packet_buffer_list = lib_alloc_packet_buffer_list(param_data_packets_per_block+param_fec_packets_per_block, MAX_PACKET_LENGTH);
	}


	rx_status = status_memory_open(param_port);
	rx_status->wifi_adapter_cnt = num_interfaces;

	for(;;) { 
		fd_set readset;
		struct timeval to;

		to.tv_sec = 0;
		to.tv_usec = 1e5;
	
		FD_ZERO(&readset);
		for(i=0; i<num_interfaces; ++i)
			FD_SET(interfaces[i].selectable_fd, &readset);

		int n = select(30, &readset, NULL, NULL, &to);

		for(i=0; i<num_interfaces; ++i) {
			if(n == 0)
				break;

			if(FD_ISSET(interfaces[i].selectable_fd, &readset)) {
                process_packet(interfaces + i, block_buffer_list, i, param_data_packets_per_block, param_fec_packets_per_block, param_packet_length, rx_status, param_block_buffers,0,0);
			}
		}

	}

	return (0);
}
