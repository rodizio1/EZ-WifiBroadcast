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
 *
 contributor : Philippe Crochat - Anemos Technologies (pcrochat@anemos-technologies.com)
 */
#pragma once

#include "fec.h"


#include "lib.h"
#include "wifibroadcast.h"
#include "radiotap.h"

#define MAX_PACKET_LENGTH 4192
#define MAX_USER_PACKET_LENGTH 1450
#define MAX_DATA_OR_FEC_PACKETS_PER_BLOCK 32
#define RC_COMMAND_CHANNEL_BLOCK_SIZE 26

#define DEBUG 0
#define debug_print(fmt, ...) \
            do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

// this is where we store a summary of the
// information from the radiotap header

typedef struct  {
	int m_nChannel;
	int m_nChannelFlags;
	int m_nRate;
	int m_nAntenna;
	int m_nRadiotapFlags;
} __attribute__((packed)) PENUMBRA_RADIOTAP_DATA;

typedef struct {
	pcap_t *ppcap;
	int selectable_fd;
	int n80211HeaderLength;
} monitor_interface_t;

typedef struct {
	int block_num;
	packet_buffer_t *packet_buffer_list;
} block_buffer_t;


void open_and_configure_interface(const char *name, monitor_interface_t *interface);
void block_buffer_list_reset(block_buffer_t *block_buffer_list, size_t block_buffer_list_len, int block_buffer_len, int param_data_packets_per_block, int param_fec_packets_per_block);
int process_payload(uint8_t *data, size_t data_len, int crc_correct, block_buffer_t *block_buffer_list, int adapter_no, int param_data_packets_per_block, int param_fec_packets_per_block, int param_block_buffers, wifibroadcast_rx_status_t *rx_status, int param_packet_length, void (*treatment)(uint8_t *, int));
int process_packet(monitor_interface_t *interface, block_buffer_t *block_buffer_list, int adapter_no, int param_data_packets_per_block, int param_fec_packets_per_block, int param_packet_length, wifibroadcast_rx_status_t *rx_status, int param_block_buffers, void (*treatment)(uint8_t *, int));
wifibroadcast_rx_status_t *status_memory_open(int param_port);
void status_memory_init(wifibroadcast_rx_status_t *s);

