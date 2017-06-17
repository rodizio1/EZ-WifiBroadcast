// rssi_forward_stoud by Rodizio.
// reads rssi from shared memory and writes it to stdout
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



wifibroadcast_rx_status_t *status_memory_open(char* shm_file) {
	int fd;

	for(;;) {
		fd = shm_open(shm_file, O_RDWR, S_IRUSR | S_IWUSR);
		if(fd > 0) { break; }
		printf("Waiting for rx to be started ...\n");
		usleep(1e6);
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


int main(int argc, char *argv[]) {
	wifibroadcast_rx_status_t *t = status_memory_open(argv[1]);
	int best_dbm = 0;
	int lostpackets = 0;
	int cardcounter = 0;
	int number_cards = t->wifi_adapter_cnt;

	for(;;) {
        	best_dbm = -128;
	        for(cardcounter=0; cardcounter<number_cards; ++cardcounter) {
            	    if (best_dbm < t->adapter[cardcounter].current_signal_dbm) best_dbm = t->adapter[cardcounter].current_signal_dbm;
		}
		lostpackets = t->lost_packet_cnt;
		write(STDOUT_FILENO,&best_dbm,1);
		write(STDOUT_FILENO,&lostpackets,4);
		usleep(100000);
	}
	return 0;
}
