#include "rx_lib.h"

int max_block_num = -1;

void status_memory_init(wifibroadcast_rx_status_t *s) {
	s->received_block_cnt = 0;
	s->damaged_block_cnt = 0;
	s->tx_restart_cnt = 0;
	s->wifi_adapter_cnt = 0;

	int i;
	for(i=0; i<MAX_PENUMBRA_INTERFACES; ++i) {
		s->adapter[i].received_packet_cnt = 0;
		s->adapter[i].wrong_crc_cnt = 0;
		s->adapter[i].current_signal_dbm = 0;
	}
}

wifibroadcast_rx_status_t *status_memory_open(int param_port) {
	char buf[128];
	int fd;
	
	sprintf(buf, "/wifibroadcast_rx_status_%d", param_port);
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

void open_and_configure_interface(const char *name, int port, monitor_interface_t *interface) {
	struct bpf_program bpfprogram;
	char szProgram[512];
	char szErrbuf[PCAP_ERRBUF_SIZE];
	// open the interface in pcap

	szErrbuf[0] = '\0';
	interface->ppcap = pcap_open_live(name, 2048, 1, -1, szErrbuf);
	if (interface->ppcap == NULL) {
		fprintf(stderr, "Unable to open interface %s in pcap: %s\n",
		    name, szErrbuf);
		exit(1);
	}
	

	if(pcap_setnonblock(interface->ppcap, 1, szErrbuf) < 0) {
		fprintf(stderr, "Error setting %s to nonblocking mode: %s\n", name, szErrbuf);
	}

	int nLinkEncap = pcap_datalink(interface->ppcap);

	switch (nLinkEncap) {

		case DLT_PRISM_HEADER:
			fprintf(stderr, "DLT_PRISM_HEADER Encap\n");
			interface->n80211HeaderLength = 0x20; // ieee80211 comes after this
			sprintf(szProgram, "radio[0x4a:4]==0x13223344 && radio[0x4e:2] == 0x55%.2x", port);
			break;

		case DLT_IEEE802_11_RADIO:
			fprintf(stderr, "DLT_IEEE802_11_RADIO Encap\n");
			interface->n80211HeaderLength = 0x18; // ieee80211 comes after this
			sprintf(szProgram, "ether[0x0a:4]==0x13223344 && ether[0x0e:2] == 0x55%.2x", port);
			break;

		default:
			fprintf(stderr, "!!! unknown encapsulation on %s !\n", name);
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


void block_buffer_list_reset(block_buffer_t *block_buffer_list, size_t block_buffer_list_len, int block_buffer_len, int param_data_packets_per_block, int param_fec_packets_per_block) {
    int i;
    block_buffer_t *rb = block_buffer_list;

    for(i=0; i<block_buffer_list_len; ++i) {
        rb->block_num = -1;

        int j;
        packet_buffer_t *p = rb->packet_buffer_list;
        for(j=0; j<param_data_packets_per_block+param_fec_packets_per_block; ++j) {
            p->valid = 0;
            p->crc_correct = 0;
            p->len = 0;
            p++;
        }

        rb++;
    }
}

void process_payload(uint8_t *data, size_t data_len, int crc_correct, block_buffer_t *block_buffer_list, int adapter_no, int param_data_packets_per_block, int param_fec_packets_per_block, int param_block_buffers, wifibroadcast_rx_status_t *rx_status, int param_packet_length, int receive_rc_command, int fd_serial)
{
    wifi_packet_header_t *wph;
    int block_num;
    int packet_num;
    int i,j;
    
    wph = (wifi_packet_header_t*)data;
    data += sizeof(wifi_packet_header_t);
    data_len -= sizeof(wifi_packet_header_t);

    block_num = wph->sequence_number / (param_data_packets_per_block+param_fec_packets_per_block);//if aram_data_packets_per_block+param_fec_packets_per_block would be limited to powers of two, this could be replaced by a logical AND operation

    //debug_print("adap %d rec %x blk %x crc %d len %d\n", adapter_no, wph->sequence_number, block_num, crc_correct, data_len);
    //printf("block number : %d\n", block_num);

    //we have received a block number that exceeds the currently seen ones -> we need to make room for this new block
    //or we have received a block_num that is several times smaller than the current window of buffers -> this indicated that either the window is too small or that the transmitter has been restarted
    int tx_restart = (block_num + 128*param_block_buffers < max_block_num);
    if((block_num > max_block_num || tx_restart) && crc_correct) {
        if(tx_restart) {
						rx_status->tx_restart_cnt++;

            fprintf(stderr, "TX RESTART: Detected blk %x that lies outside of the current retr block buffer window (max_block_num = %x) (if there was no tx restart, increase window size via -d)\n", block_num, max_block_num);


            block_buffer_list_reset(block_buffer_list, param_block_buffers, param_data_packets_per_block + param_fec_packets_per_block, param_data_packets_per_block, param_fec_packets_per_block);
        }

        //first, find the minimum block num in the buffers list. this will be the block that we replace
        int min_block_num = INT_MAX;
        int min_block_num_idx;
        for(i=0; i<param_block_buffers; ++i) {
            if(block_buffer_list[i].block_num < min_block_num) {
                min_block_num = block_buffer_list[i].block_num;
                min_block_num_idx = i;
            }
        }

        //debug_print("removing block %x at index %i for block %x\n", min_block_num, min_block_num_idx, block_num);

        packet_buffer_t *packet_buffer_list = block_buffer_list[min_block_num_idx].packet_buffer_list;
        int last_block_num = block_buffer_list[min_block_num_idx].block_num;

        if(last_block_num != -1) {
            rx_status->received_block_cnt++;

            //we have both pointers to the packet buffers (to get information about crc and vadility) and raw data pointers for fec_decode
            packet_buffer_t *data_pkgs[MAX_DATA_OR_FEC_PACKETS_PER_BLOCK];
            packet_buffer_t *fec_pkgs[MAX_DATA_OR_FEC_PACKETS_PER_BLOCK];
            uint8_t *data_blocks[MAX_DATA_OR_FEC_PACKETS_PER_BLOCK];
            uint8_t *fec_blocks[MAX_DATA_OR_FEC_PACKETS_PER_BLOCK];
            int datas_missing = 0, datas_corrupt = 0, fecs_missing = 0, fecs_corrupt = 0;
            int di = 0, fi = 0;


            //first, split the received packets into DATA a FEC packets and count the damaged packets
            i = 0;
            while(di < param_data_packets_per_block || fi < param_fec_packets_per_block) {
                if(di < param_data_packets_per_block) {
                    data_pkgs[di] = packet_buffer_list + i++;
                    data_blocks[di] = data_pkgs[di]->data;
                    if(!data_pkgs[di]->valid)
                        datas_missing++;
                    if(data_pkgs[di]->valid && !data_pkgs[di]->crc_correct)
                        datas_corrupt++;
                    di++;
                }

                if(fi < param_fec_packets_per_block) {
                    fec_pkgs[fi] = packet_buffer_list + i++;
                    if(!fec_pkgs[fi]->valid)
                        fecs_missing++;

                    if(fec_pkgs[fi]->valid && !fec_pkgs[fi]->crc_correct)
                        fecs_corrupt++;

                    fi++;
                }
            }

            const int good_fecs_c = param_fec_packets_per_block - fecs_missing - fecs_corrupt;
            const int datas_missing_c = datas_missing;
            const int datas_corrupt_c = datas_corrupt;
            const int fecs_missing_c = fecs_missing;
            const int fecs_corrupt_c = fecs_corrupt;

            int good_fecs = good_fecs_c;
            //the following three fields are infos for fec_decode
            unsigned int fec_block_nos[MAX_DATA_OR_FEC_PACKETS_PER_BLOCK];
            unsigned int erased_blocks[MAX_DATA_OR_FEC_PACKETS_PER_BLOCK];
            unsigned int nr_fec_blocks = 0;


#if DEBUG
            if(datas_missing_c + datas_corrupt_c > good_fecs_c) {
                int x;

                for(x=0;x<param_data_packets_per_block; ++x) {
                    if(data_pkgs[x]->valid) {
                        if(data_pkgs[x]->crc_correct)
                            fprintf(stderr, "v");
                        else
                            fprintf(stderr, "c");
                    }
                    else
                        fprintf(stderr, "m");
                }

                fprintf(stderr, " ");

                for(x=0;x<param_fec_packets_per_block; ++x) {
                    if(fec_pkgs[x]->valid) {
                        if(fec_pkgs[x]->crc_correct)
                            fprintf(stderr, "v");
                        else
                            fprintf(stderr, "c");
                    }
                    else
                        fprintf(stderr, "m");
                }

                fprintf(stderr, "\n");
            }
#endif

            fi = 0;
            di = 0;

            //look for missing DATA and replace them with good FECs
            while(di < param_data_packets_per_block && fi < param_fec_packets_per_block) {
                //if this data is fine we go to the next
                if(data_pkgs[di]->valid && data_pkgs[di]->crc_correct) {
                    di++;
                    continue;
                }

                //if this DATA is corrupt and there are less good fecs than missing datas we cannot do anything for this data
                if(data_pkgs[di]->valid && !data_pkgs[di]->crc_correct && good_fecs <= datas_missing) {
                    di++;
                    continue;
                }

                //if this FEC is not received we go on to the next
                if(!fec_pkgs[fi]->valid) {
                    fi++;
                    continue;
                }

                //if this FEC is corrupted and there are more lost packages than good fecs we should replace this DATA even with this corrupted FEC
                if(!fec_pkgs[fi]->crc_correct && datas_missing > good_fecs) {
                    fi++;
                    continue;
                }


                if(!data_pkgs[di]->valid)
                    datas_missing--;
                else if(!data_pkgs[di]->crc_correct)
                    datas_corrupt--;

                if(fec_pkgs[fi]->crc_correct)
                    good_fecs--;

                //at this point, data is invalid and fec is good -> replace data with fec
                erased_blocks[nr_fec_blocks] = di;
                fec_block_nos[nr_fec_blocks] = fi;
                fec_blocks[nr_fec_blocks] = fec_pkgs[fi]->data;
                di++;
                fi++;
                nr_fec_blocks++;
            }


            int reconstruction_failed = datas_missing_c + datas_corrupt_c > good_fecs_c;

            if(reconstruction_failed) {
                //we did not have enough FEC packets to repair this block
                rx_status->damaged_block_cnt++;
                //fprintf(stderr, "Could not fully reconstruct block %x! Damage rate: %f (%d / %d blocks)\n", last_block_num, 1.0 * rx_status->damaged_block_cnt / rx_status->received_block_cnt, rx_status->damaged_block_cnt, rx_status->received_block_cnt);
                debug_print("Data mis: %d\tData corr: %d\tFEC mis: %d\tFEC corr: %d\n", datas_missing_c, datas_corrupt_c, fecs_missing_c, fecs_corrupt_c);
            }


            //decode data and write it to STDOUT
            fec_decode((unsigned int) param_packet_length, data_blocks, param_data_packets_per_block, fec_blocks, fec_block_nos, erased_blocks, nr_fec_blocks);
            for(i=0; i<param_data_packets_per_block; ++i) {
                payload_header_t *ph = (payload_header_t*)data_blocks[i];

                if(!reconstruction_failed || data_pkgs[i]->valid) {
                    //if reconstruction did fail, the data_length value is undefined. better limit it to some sensible value
                    if(ph->data_length > param_packet_length)
                        ph->data_length = param_packet_length;
                    	
                    	if(receive_rc_command==0)
                    	{
                    		write(STDOUT_FILENO, data_blocks[i] + sizeof(payload_header_t), ph->data_length);
                    		fflush(stdout);
                    	}
                    	else
                    	{
                    		printf("receive_rc_command %d\n", receive_rc_command);
                    		printf("\n");
                    		
                    		for(j=0;j<5;j++)
                    		{
                    			printf("%c", *(data_blocks[i] + sizeof(payload_header_t)+j));
                    		}
                    		
                    		for(j=0;j<8;j++)
                    		{
                    			printf("%d : %d, ", j, ((*(data_blocks[i] + sizeof(payload_header_t)+5+j*2+1))<<8)+*(data_blocks[i] + sizeof(payload_header_t)+5+j*2));
                    		}
                    		
                    		write(fd_serial, data_blocks[i] + sizeof(payload_header_t), param_packet_length);
                    		
                    		//wait for serialport to send
                    		tcdrain(fd_serial);
                    	}
                }
            }


            //reset buffers
            for(i=0; i<param_data_packets_per_block + param_fec_packets_per_block; ++i) {
                packet_buffer_t *p = packet_buffer_list + i;
                p->valid = 0;
                p->crc_correct = 0;
                p->len = 0;
            }
        }

    block_buffer_list[min_block_num_idx].block_num = block_num;
    max_block_num = block_num;
    }


    //find the buffer into which we have to write this packet
    block_buffer_t *rbb = block_buffer_list;
    for(i=0; i<param_block_buffers; ++i) {
        if(rbb->block_num == block_num) {
            break;
        }
        rbb++;
    }

    //check if we have actually found the corresponding block. this could not be the case due to a corrupt packet
    if(i != param_block_buffers) {
        packet_buffer_t *packet_buffer_list = rbb->packet_buffer_list;
        packet_num = wph->sequence_number % (param_data_packets_per_block+param_fec_packets_per_block); //if retr_block_size would be limited to powers of two, this could be replace by a locical and operation

        //only overwrite packets where the checksum is not yet correct. otherwise the packets are already received correctly
        if(packet_buffer_list[packet_num].crc_correct == 0) {
            memcpy(packet_buffer_list[packet_num].data, data, data_len);
            packet_buffer_list[packet_num].len = data_len;
            packet_buffer_list[packet_num].valid = 1;
            packet_buffer_list[packet_num].crc_correct = crc_correct;
        }
    }

}


void process_packet(monitor_interface_t *interface, block_buffer_t *block_buffer_list, int adapter_no, int param_data_packets_per_block, int param_fec_packets_per_block, int param_packet_length, wifibroadcast_rx_status_t *rx_status, int param_block_buffers, int receive_rc_command, int fd_serial) {
		struct pcap_pkthdr * ppcapPacketHeader = NULL;
		struct ieee80211_radiotap_iterator rti;
		PENUMBRA_RADIOTAP_DATA prd;
		u8 payloadBuffer[MAX_PACKET_LENGTH];
		u8 *pu8Payload = payloadBuffer;
		int bytes;
		int n;
		int retval;
		int u16HeaderLen;

		// receive


		retval = pcap_next_ex(interface->ppcap, &ppcapPacketHeader,
		    (const u_char**)&pu8Payload);

		if (retval < 0) {
			fprintf(stderr, "Socket broken\n");
			fprintf(stderr, "%s\n", pcap_geterr(interface->ppcap));
			exit(1);
		}                                 

		//if(retval == 0)
		//	fprintf(stderr, "retval = 0\n");

		if (retval != 1)
			return;


		u16HeaderLen = (pu8Payload[2] + (pu8Payload[3] << 8));

		
		if (ppcapPacketHeader->len <
		    (u16HeaderLen + interface->n80211HeaderLength))
			return;

		bytes = ppcapPacketHeader->len -
			(u16HeaderLen + interface->n80211HeaderLength);
			
		if (bytes < 0)
			return;

		if (ieee80211_radiotap_iterator_init(&rti,
		    (struct ieee80211_radiotap_header *)pu8Payload,
		    ppcapPacketHeader->len) < 0)
			return;

		while ((n = ieee80211_radiotap_iterator_next(&rti)) == 0) {

			switch (rti.this_arg_index) {
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

			case IEEE80211_RADIOTAP_FLAGS:
				prd.m_nRadiotapFlags = *rti.this_arg;
				break;

			case IEEE80211_RADIOTAP_DBM_ANTSIGNAL:
				rx_status->adapter[adapter_no].current_signal_dbm = (int8_t)(*rti.this_arg);
				break;

			}
		}
		
		pu8Payload += u16HeaderLen + interface->n80211HeaderLength;

		if (prd.m_nRadiotapFlags & IEEE80211_RADIOTAP_F_FCS)
			bytes -= 4;


        int checksum_correct = (prd.m_nRadiotapFlags & 0x40) == 0;

		if(!checksum_correct)
			rx_status->adapter[adapter_no].wrong_crc_cnt++;

		rx_status->adapter[adapter_no].received_packet_cnt++;
		
		if(rx_status->adapter[adapter_no].received_packet_cnt % 1024 == 0) {
//			fprintf(stderr, "Signal (card %d): %ddBm\n", adapter_no, rx_status->adapter[adapter_no].current_signal_dbm);
		}

		rx_status->last_update = time(NULL);

        process_payload(pu8Payload, bytes, checksum_correct, block_buffer_list, adapter_no, param_data_packets_per_block, param_fec_packets_per_block, param_block_buffers, rx_status, param_packet_length, receive_rc_command, fd_serial);
}