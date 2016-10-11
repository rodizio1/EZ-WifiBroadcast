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


void open_and_configure_interface(const char *name, int port, monitor_interface_t *interface);
void block_buffer_list_reset(block_buffer_t *block_buffer_list, size_t block_buffer_list_len, int block_buffer_len, int param_data_packets_per_block, int param_fec_packets_per_block);
void process_payload(uint8_t *data, size_t data_len, int crc_correct, block_buffer_t *block_buffer_list, int adapter_no, int param_data_packets_per_block, int param_fec_packets_per_block, int param_block_buffers, wifibroadcast_rx_status_t *rx_status, int param_packet_length, int receive_rc_command, int fd_serial);
void process_packet(monitor_interface_t *interface, block_buffer_t *block_buffer_list, int adapter_no, int param_data_packets_per_block, int param_fec_packets_per_block, int param_packet_length, wifibroadcast_rx_status_t *rx_status, int param_block_buffers, int receive_rc_command, int fd_serial);
wifibroadcast_rx_status_t *status_memory_open(int param_port);
void status_memory_init(wifibroadcast_rx_status_t *s);

