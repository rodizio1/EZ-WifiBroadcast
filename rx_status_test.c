#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <resolv.h>
#include <string.h>
#include <utime.h>
#include <unistd.h>
#include <getopt.h>
#include <pcap.h>
#include <endian.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "lib.h"

wifibroadcast_rx_status_t *status_memory_open(void) {
	
	int fd;

	for(;;) {
		fd = shm_open("/wifibroadcast_rx_status_0", O_RDWR, S_IRUSR | S_IWUSR);
		if(fd > 0) {
			break;
		}
		printf("\033[1;1H");
		printf("Waiting for cards to come up ...\n");
		usleep(1e5);
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
	
	
	return (wifibroadcast_rx_status_t*)retval;

}


int main(void) {

//	printf("\033[2J");

	wifibroadcast_rx_status_t *t = status_memory_open();

	for(;;) {
		printf("\033[1;1H");
		printf("Card1: %ddBm(%d), Card2: %ddBm(%d), Card3: %ddBm(%d). Total %d/%d blocks bad                  \n", t->adapter[0].current_signal_dbm, t->adapter[0].received_packet_cnt, t->adapter[1].current_signal_dbm, t->adapter[1].received_packet_cnt, t->adapter[2].current_signal_dbm, t->adapter[2].received_packet_cnt, t->damaged_block_cnt, t->received_block_cnt);
//		printf("Card1: %ddBm(%d/%d), Card2: %ddBm(%d/%d), Card3: %ddBm(%d/%d). Total %d/%d blocks bad           \n", t->adapter[0].current_signal_dbm, t->adapter[0].wrong_crc_cnt, t->adapter[0].received_packet_cnt, t->adapter[1].current_signal_dbm, t->adapter[1].wrong_crc_cnt, t->adapter[1].received_packet_cnt, t->adapter[2].current_signal_dbm, t->adapter[2].wrong_crc_cnt, t->adapter[2].received_packet_cnt, t->damaged_block_cnt, t->received_block_cnt);
//		printf("Wifi cards:\t\t%d\n", t->wifi_adapter_cnt);
//		printf("TX restarts:\t\t%dn", t->tx_restart_cnt);
		usleep(1e5);
	}

	return 0;
}
