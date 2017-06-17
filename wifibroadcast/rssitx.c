// rssitx by Rodizio
// reads rssi from shared mem and sends it out on wifi interfaces (for R/C rssi)
#include <stdlib.h>
#include <stdio.h>
#include <sys/resource.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pcap.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <netpacket/packet.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <arpa/inet.h>
#include <string.h>
#include <pcap.h>
#include <getopt.h>
#include "lib.h"

char *ifname = NULL;
int flagHelp = 0;

int sock=0;
int socks[5];

void usage(void)
{
    printf(
        "rssitx by Rodizio.\n"
        "\n"
        "Usage: rssitx <interface>\n"
        "\n"
        "Example:\n"
        "  rssitx wlan0\n"
        "\n");
    exit(1);
}



static int open_sock (char *ifname) {
    struct sockaddr_ll ll_addr;
    struct ifreq ifr;

    sock = socket (PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock == -1) {
	fprintf(stderr, "Error:\tSocket failed\n");
	exit(1);
    }

    ll_addr.sll_family = AF_PACKET;
    ll_addr.sll_protocol = 0;
    ll_addr.sll_halen = ETH_ALEN;

    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
	fprintf(stderr, "Error:\tioctl(SIOCGIFINDEX) failed\n");
	exit(1);
    }

    ll_addr.sll_ifindex = ifr.ifr_ifindex;

    if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
	fprintf(stderr, "Error:\tioctl(SIOCGIFHWADDR) failed\n");
	exit(1);
    }

    memcpy(ll_addr.sll_addr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);

    if (bind (sock, (struct sockaddr *)&ll_addr, sizeof(ll_addr)) == -1) {
	fprintf(stderr, "Error:\tbind failed\n");
	close(sock);
	exit(1);
    }

    if (sock == -1 ) {
        fprintf(stderr,
        "Error:\tCannot open socket\n"
        "Info:\tMust be root with an 802.11 card with RFMON enabled\n");
        exit(1);
    }

    return sock;
}


void sendRSSI(int sock, telemetry_data_t *td) {

	struct framedata_s {
	    uint8_t rt1;
	    uint8_t rt2;
	    uint8_t rt3;
	    uint8_t rt4;
	    uint8_t rt5;
	    uint8_t rt6;
	    uint8_t rt7;
	    uint8_t rt8;

	    uint8_t rt9;
	    uint8_t rt10;
	    uint8_t rt11;
	    uint8_t rt12;

	    uint8_t fc1;
	    uint8_t fc2;
	    uint8_t dur1;
	    uint8_t dur2;

	    uint8_t mac1_1; // FF
	    uint8_t port; // port
	    uint8_t mac1_3;
	    uint8_t mac1_4;
	    uint8_t mac1_5;
	    uint8_t mac1_6;

	    uint8_t mac2_1;
	    uint8_t mac2_2;
	    uint8_t mac2_3;
	    uint8_t mac2_4;
	    uint8_t mac2_5;
	    uint8_t mac2_6;

	    uint8_t mac3_1;
	    uint8_t mac3_2;
	    uint8_t mac3_3;
	    uint8_t mac3_4;
	    uint8_t mac3_5;
	    uint8_t mac3_6;

	    uint8_t ieeeseq1;
	    uint8_t ieeeseq2;

	    int signal;
	    int lostpackets;
	}  __attribute__ ((__packed__));

	struct framedata_s framedata;

	framedata.rt1 = 0; // <-- radiotap version
	framedata.rt2 = 0; // <-- radiotap version

	framedata.rt3 = 12; // <- radiotap header length
	framedata.rt4 = 0; // <- radiotap header length

	framedata.rt5 = 4; // <-- radiotap present flags
	framedata.rt6 = 128; // <-- radiotap present flags
	framedata.rt7 = 0; // <-- radiotap present flags
	framedata.rt8 = 0; // <-- radiotap present flags

	framedata.rt9 = 24; // <-- radiotap rate
	framedata.rt10 = 0; // <-- radiotap stuff
	framedata.rt11 = 0; // <-- radiotap stuff
	framedata.rt12 = 0; // <-- radiotap stuff

	framedata.fc1 = 8; // <-- frame control field 0x08 = 8 (data frame)    180 = rts frame
	framedata.fc2 = 191; // <-- frame control field 0xbf = 191
	framedata.dur1 = 0; // <-- duration
	framedata.dur2 = 0; // <-- duration

	framedata.mac1_1 = 255 ; // 0xff
	framedata.port = 8; // port
	framedata.mac1_3 = 14;
	framedata.mac1_4 = 0;
	framedata.mac1_5 = 1;
	framedata.mac1_6 = 0;

	framedata.mac2_1 = 12;
	framedata.mac2_2 = 0;
	framedata.mac2_3 = 21;
	framedata.mac2_4 = 0;
	framedata.mac2_5 = 0;
	framedata.mac2_6 = 0;

	framedata.mac3_1 = 0;
	framedata.mac3_2 = 0;
	framedata.mac3_3 = 0;
	framedata.mac3_4 = 0;
	framedata.mac3_5 = 0;
	framedata.mac3_6 = 0;

	framedata.ieeeseq1 = 0;
	framedata.ieeeseq2 = 0;

	framedata.signal = 0;
	framedata.lostpackets = td->rx_status->lost_packet_cnt;

	if(td->rx_status != NULL) {

		int best_dbm = -128;
    		int cardcounter = 0;
    		int number_cards = td->rx_status->wifi_adapter_cnt;

		// ignore cards with 0dbm = no signal
                for(cardcounter=0; cardcounter<number_cards; ++cardcounter) {
                    if (best_dbm < td->rx_status->adapter[cardcounter].current_signal_dbm) best_dbm = td->rx_status->adapter[cardcounter].current_signal_dbm;
                }


                for(cardcounter=0; cardcounter<number_cards; ++cardcounter) {
                    if (best_dbm < td->rx_status->adapter[cardcounter].current_signal_dbm) best_dbm = td->rx_status->adapter[cardcounter].current_signal_dbm;
                }

		framedata.signal = best_dbm;
		framedata.lostpackets = td->rx_status->lost_packet_cnt;
	}

	printf("signal: %d\n", framedata.signal);
	if (write(socks[0], &framedata, 44) < 0 ) fprintf(stderr, "!");
}



wifibroadcast_rx_status_t *telemetry_wbc_status_memory_open(void) {

    int fd = 0;
    int sharedmem = 0;

    while(sharedmem == 0) {
        fd = shm_open("/wifibroadcast_rx_status_5", O_RDWR, S_IRUSR | S_IWUSR);
	    if(fd < 0) {
		fprintf(stderr, "Could not open wifibroadcast rx R/C status - will try again ...\n");
	    } else {
		sharedmem = 1;
	    }
	    usleep(150000);
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

return 0;
}

void telemetry_init(telemetry_data_t *td) {
    td->rx_status = telemetry_wbc_status_memory_open();
}




int main (int argc, char *argv[]) {
    int done = 1;

	socks[0] = open_sock(argv[1]);

	// init RSSI shared memory
	telemetry_data_t td;
	telemetry_init(&td);

	while (done) {
		sendRSSI(sock,&td);
		usleep(100000);
	}
	return 0;
}
