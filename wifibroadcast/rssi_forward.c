// rssi_forward by Rodizio.
// reads video rssi from shared mem and sends it out via UDP (for FPV_VR app)
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

struct sockaddr_in si_other_rssi;
int s, s_rssi, slen_rssi=sizeof(si_other_rssi);

wifibroadcast_rx_status_t *status_memory_open(char* shm_file) {
	int fd;

	for(;;) {
		fd = shm_open(shm_file, O_RDWR, S_IRUSR | S_IWUSR);
		if(fd > 0) {
			break;
		}
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
	uint8_t rssi;
	si_other_rssi.sin_family = AF_INET;
	si_other_rssi.sin_port = htons(5003);
	si_other_rssi.sin_addr.s_addr = inet_addr(argv[2]);
	memset(si_other_rssi.sin_zero, '\0', sizeof(si_other_rssi.sin_zero));

	wifibroadcast_rx_status_t *t = status_memory_open(argv[1]);
	int best_dbm = 0;
	int cardcounter = 0;
	int number_cards = t->wifi_adapter_cnt;

	if ((s_rssi=socket(PF_INET, SOCK_DGRAM, 0))==-1) printf("ERROR: Could not create UDP socket!");

	for(;;) {
		best_dbm = -128;
		for(cardcounter=0; cardcounter<number_cards; ++cardcounter) {
		    if (best_dbm < t->adapter[cardcounter].current_signal_dbm) best_dbm = t->adapter[cardcounter].current_signal_dbm;
		}
		rssi = best_dbm;
		if (sendto(s_rssi, &rssi, 1, 0, (struct sockaddr*)&si_other_rssi, slen_rssi)==-1) printf("ERROR: Could not send RSSI data!");
		usleep(100000);
	}
	return 0;
}
