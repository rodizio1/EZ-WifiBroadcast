#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <stdio.h>
#include <stdlib.h>
#include "telemetry.h"
#include "osdconfig.h"

void telemetry_init(telemetry_data_t *td) {
	td->validmsgsrx = 0;
	td->datarx = 0;
	td->voltage = 0;
	td->ampere = 0;
	td->altitude = 0;
	td->baro_altitude = 0;
	td->longitude = 0;
	td->latitude = 0;
	td->heading = 0;
	td->speed = 0;
	td->airspeed = 0;
	td->x = 0;
	td->y = 0;
	td->z = 0;
	td->ew = 0;
	td->ns = 0;

//	td->hdop = 0;
	td->sats = 0;
	td->fix = 0;

#ifdef LTM
	td->roll = 0;
	td->pitch = 0;
	td->rssi = 0;
	td->airspeed = 0;
	td->uav_arm = 0;
	td->uav_failsafe = 0;
	td->uav_flightmode = 0;
	td->home_longitude = 0;
	td->home_latitude = 0;
	td->home_altitude = 0;
	td->osdon = 0;
	td->home_fix = 0;
#endif

#ifdef VIDEO_RSSI
	td->rx_status = telemetry_wbc_status_memory_open();
#endif

#ifdef OSD_RSSI
	td->rx_status_osd = telemetry_wbc_status_memory_open_osd();
#endif

#ifdef RC_RSSI
	td->rx_status_rc = telemetry_wbc_status_memory_open_rc();
#endif
}

#ifdef VIDEO_RSSI
wifibroadcast_rx_status_t *telemetry_wbc_status_memory_open(void) {

        int fd = 0;
	int sharedmem = 0;

	while(sharedmem == 0) {
	    fd = shm_open("/wifibroadcast_rx_status_0", O_RDWR, S_IRUSR | S_IWUSR);
    	    if(fd < 0) {
                fprintf(stderr, "Could not open wifibroadcast rx status - will try again ...\n");
    	    } else {
		sharedmem = 1;
	    }
	    usleep(100000);
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
#endif

#ifdef OSD_RSSI
wifibroadcast_rx_status_t_osd *telemetry_wbc_status_memory_open_osd(void) {

        int fd = 0;
	int sharedmem = 0;

	while(sharedmem == 0) {
	    fd = shm_open("/wifibroadcast_rx_status_1", O_RDWR, S_IRUSR | S_IWUSR);
    	    if(fd < 0) {
                fprintf(stderr, "Could not open wifibroadcast rx osd status - will try again ...\n");
    	    } else {
		sharedmem = 1;
	    }
	    usleep(100000);
	}

        if (ftruncate(fd, sizeof(wifibroadcast_rx_status_t_osd)) == -1) {
                perror("ftruncate");
                exit(1);
        }

        void *retval = mmap(NULL, sizeof(wifibroadcast_rx_status_t_osd), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (retval == MAP_FAILED) {
                perror("mmap");
                exit(1);
        }

        return (wifibroadcast_rx_status_t_osd*)retval;
}
#endif

#ifdef RC_RSSI
wifibroadcast_rx_status_t_rc *telemetry_wbc_status_memory_open_rc(void) {
        int fd = 0;
	int sharedmem = 0;

	while(sharedmem == 0) {
	    fd = shm_open("/wifibroadcast_rx_status_9", O_RDWR, S_IRUSR | S_IWUSR);
    	    if(fd < 0) {
                fprintf(stderr, "Could not open wifibroadcast rx R/C status - will try again ...\n");
    	    } else {
		sharedmem = 1;
	    }
	    usleep(100000);
	}

        if (ftruncate(fd, sizeof(wifibroadcast_rx_status_t_osd)) == -1) {
                perror("ftruncate");
                exit(1);
        }

        void *retval = mmap(NULL, sizeof(wifibroadcast_rx_status_t_osd), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (retval == MAP_FAILED) {
                perror("mmap");
                exit(1);
        }

        return (wifibroadcast_rx_status_t_rc*)retval;
}
#endif
