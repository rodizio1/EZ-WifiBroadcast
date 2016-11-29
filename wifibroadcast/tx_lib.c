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
 contributor : Philippe Crochat - Anemos Technologies (pcrochat@anemos-technologies.com)
 */
 
#include "tx_lib.h"

static u8 u8aIeeeHeader[] = {
	0x08, 0x01, 0x00, 0x00,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x13, 0x22, 0x33, 0x44, 0x55, 0x66, //src mac address
	0x13, 0x22, 0x33, 0x44, 0x55, 0x66, //dst mac address
	0x10, 0x86,
};

/* this is the template radiotap header we send packets out with */

u8 u8aRadiotapHeader[] = {

	0x00, 0x00, // <-- radiotap version
	0x0c, 0x00, // <- radiotap header lengt
	0x04, 0x80, 0x00, 0x00, // <-- radiotap present flags
	0x24, // datarate 18Mbit
//	0x30, // datarate 24Mbit
//	0x48, // datarate 36Mbit
//	0x60, // datarate 48Mbit
	0x0, 
	0x18, 0x00 
};

void pb_transmit_packet(pcap_t *ppcap, int seq_nr, uint8_t *packet_transmit_buffer, int packet_header_len, const uint8_t *packet_data, int packet_length) {
    //add header outside of FEC
   // int i;
    wifi_packet_header_t *wph = (wifi_packet_header_t*)(packet_transmit_buffer + packet_header_len);
    wph->sequence_number = seq_nr;

    //copy data
    memcpy(packet_transmit_buffer + packet_header_len + sizeof(wifi_packet_header_t), packet_data, packet_length);

   /* for(i=0;i<packet_length;i++)
    	printf("%d ", *(packet_transmit_buffer + packet_header_len + sizeof(wifi_packet_header_t)+i));

    printf("\n");*/
    
    int plen = packet_length + packet_header_len + sizeof(wifi_packet_header_t);
    int r = pcap_inject(ppcap, packet_transmit_buffer, plen);
    if (r != plen) {
        pcap_perror(ppcap, "Trouble injecting packet");
        exit(1);
    }
}




void pb_transmit_block(packet_buffer_t *pbl, pcap_t *ppcap, int *seq_nr, int packet_length, uint8_t *packet_transmit_buffer, int packet_header_len, int data_packets_per_block, int fec_packets_per_block, int transmission_count) {
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
	pb += packet_header_len;


	int x;
	for(x=0; x<transmission_count; ++x) {
		//send data and FEC packets interleaved
		int di = 0; 
		int fi = 0;
		int seq_nr_tmp = *seq_nr;
		while(di < data_packets_per_block || fi < fec_packets_per_block) {
			if(di < data_packets_per_block) {
				pb_transmit_packet(ppcap, seq_nr_tmp, packet_transmit_buffer, packet_header_len, data_blocks[di], packet_length);
				seq_nr_tmp++;
				di++;
			}

			if(fi < fec_packets_per_block) {
				pb_transmit_packet(ppcap, seq_nr_tmp, packet_transmit_buffer, packet_header_len, fec_blocks[fi], packet_length);
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

int packet_header_init(uint8_t *packet_header, int data_rate, int transmitter) {
			u8 *pu8 = packet_header;
			
			//change radio tap header according to data bit rate
			switch(data_rate)
			{
				case 18:
					u8aRadiotapHeader[8]=0x24;
					break;
					
				case 24:
					u8aRadiotapHeader[8]=0x30;
					break;
					
				case 36:
					u8aRadiotapHeader[8]=0x48;
					break;
					
				case 48:
					u8aRadiotapHeader[8]=0x60;
					break;
					
				default:
					u8aRadiotapHeader[8]=0x24;
			}
		
			if(transmitter == 1)
				u8aIeeeHeader[13]=0x45;
			
			memcpy(packet_header, u8aRadiotapHeader, sizeof(u8aRadiotapHeader));
			pu8 += sizeof(u8aRadiotapHeader);
			memcpy(pu8, u8aIeeeHeader, sizeof (u8aIeeeHeader));
			pu8 += sizeof (u8aIeeeHeader);
					
			//determine the length of the header
			return pu8 - packet_header;
}

/*
void set_port_no(uint8_t *pu, uint8_t port) {
	//dirty hack: the last byte of the mac address is the port number. this makes it easy to filter out specific ports via wireshark
	pu[sizeof(u8aRadiotapHeader) + SRC_MAC_LASTBYTE] = port;
	pu[sizeof(u8aRadiotapHeader) + DST_MAC_LASTBYTE] = port;
}*/


