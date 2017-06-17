// rssi_write_shmem by Rodizio
// reads rssi from stdin and writes it to shared memory
#include "lib.h"
#include "wifibroadcast.h"

#include <time.h>
#include <sys/resource.h>


wifibroadcast_rx_status_t *rx_status = NULL;


void status_memory_init(wifibroadcast_rx_status_t *s) {
	s->received_block_cnt = 0;
	s->damaged_block_cnt = 0;
	s->tx_restart_cnt = 0;
	s->wifi_adapter_cnt = 0;

	s->adapter[0].received_packet_cnt = 0;
	s->adapter[0].wrong_crc_cnt = 0;
	s->adapter[0].current_signal_dbm = 0;
}



wifibroadcast_rx_status_t *status_memory_open(char* shm_file) {

        int fd;

        fd = shm_open(shm_file, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);

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



int main(int argc, char *argv[])
{
	rx_status = status_memory_open(argv[1]);
	for(;;) {
	    usleep(20000);
	    read(STDIN_FILENO,&rx_status->adapter[0].current_signal_dbm,1);
	    read(STDIN_FILENO,&rx_status->lost_packet_cnt,4);
	    printf("dbm: %d\n", rx_status->adapter[0].current_signal_dbm);
	    printf("lost: %d\n", rx_status->lost_packet_cnt);

	    // TODO: write to shared mem

	}
	return (0);
}
