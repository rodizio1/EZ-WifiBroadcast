// (c) Philippe Crochat - Anemos Technologies - www.anemos-technologies.com
/*   pcrochat@anemos-technologies.com
 *
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
 */

/* implements multiwii protocol on raspberry pi with standart joystick linux driver */ 
//for use in conjonction with wifibroadcast only

#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <linux/joystick.h>

#include "tx_lib.h"
#include "send_joystick_command.h"

struct timeval last_sent;
struct timeval now;
struct timeval timer_result;

int main(int argc, char *argv[])
{
	int joy_fd, *axis=NULL;
	unsigned char num_of_axis = 2;
	unsigned char num_of_buttons = 2;
	int version = 0x000800;
	char name_of_joystick[NAME_LENGTH] = "Unknown";
	
	unsigned char x;
	char *buttons=NULL;
    
    char szErrbuf[PCAP_ERRBUF_SIZE];
	pcap_t *ppcap = NULL;
	int pcnt = 0;
	uint8_t packet_transmit_buffer[MAX_PACKET_LENGTH];
	uint8_t packet_data[PACKET_LENGTH];
	uint8_t old_packet_data[PACKET_LENGTH];
	uint8_t crc;
	size_t packet_header_length = 0;
	int file_exists;
	
	int param_port = 2;
    
	if (argc < 2 || argc > 3 || !strcmp("--help", argv[1])) {
		puts("");
		puts("Usage: joystick <device>");
		puts("");
		exit(1);
	}
	
	if ((joy_fd = open(JOY_DEV, O_RDONLY)) < 0) {
		perror("joystick");
		exit(1);
	}

	gettimeofday(&last_sent, NULL);
	
	ioctl(joy_fd, JSIOCGVERSION, &version);
	ioctl(joy_fd, JSIOCGAXES, &num_of_axis);
	ioctl(joy_fd, JSIOCGBUTTONS, &num_of_buttons);
	ioctl(joy_fd, JSIOCGNAME(NAME_LENGTH), name_of_joystick);

	printf("Joystick (%s) has %d axes and %d buttons. Driver version is %d.%d.%d.\n",
	name_of_joystick, num_of_axis, num_of_buttons, version >> 16, (version >> 8) & 0xff, version & 0xff);
	
	axis = (int32_t *) calloc( num_of_axis, sizeof( int32_t ) );
	buttons = (char *) calloc( num_of_buttons+1, sizeof( char ) );

	struct js_event js;

	packet_header_length = packet_header_init(packet_transmit_buffer);
	
	// open the interface in pcap
	szErrbuf[0] = '\0';
	ppcap = pcap_open_live(argv[optind], 800, 1, 20, szErrbuf);
	if (ppcap == NULL) {
		fprintf(stderr, "Unable to open interface %s in pcap: %s\n",
		    argv[optind], szErrbuf);
		return (1);
	}

	set_port_no(packet_transmit_buffer, param_port); //set port number
	
	pcap_setnonblock(ppcap, 0, szErrbuf); /* pcap non-blocking mode */
	fcntl( joy_fd, F_SETFL, O_NONBLOCK );	/* use joystick non-blocking mode */
	
	while( 1 ) 	/* infinite loop */
	{
		file_exists=access( JOY_DEV, F_OK ); //test if joystick is still connected
		
		/* read the joystick state */
		if (read(joy_fd, &js, sizeof(struct js_event)) != sizeof(struct js_event)) {
			usleep(1000);
			//continue;
		}
		
			/* see what to do with the event */
		switch (js.type & ~JS_EVENT_INIT)
		{
			case JS_EVENT_AXIS:
				axis   [ js.number ] = js.value;
				break;
			case JS_EVENT_BUTTON:
				buttons [ js.number ] = js.value;
				break;
		}

		for(x=0;x<4;x++)
		{
			packet_data[x]=0;
		}
		
		packet_data[4] = '$';
		packet_data[5] = 'M';
		packet_data[6] = '<';
		packet_data[7] = 16; //8 channels 2 bytes
		packet_data[8] = 200; // MSP_SET_RAW_RC
		
		for(x=0;x<8;x++)
		{
			packet_data[x*2+9]=(uint8_t)((1000+((axis[CHANNEL_INDEX[x]]+32768)*1000/65535))&0xFF);
			packet_data[x*2+10]=(uint8_t)((1000+((axis[CHANNEL_INDEX[x]]+32768)*1000/65535))>>8);
		}
		
		//build crc 
		crc=0;
		for(x=7;x<25;x++)
		{
			crc^=packet_data[x];
		}
		
		packet_data[25]=crc;
		
		int transmit_packet=0;
		
		for(x=0;x<PACKET_LENGTH;x++)
		{
			if(packet_data[x]!=old_packet_data[x])
			{
				printf("%d:%d:%d\n", x, packet_data[x], old_packet_data[x]);
				transmit_packet=1;
				break;
			}
		}
		
		gettimeofday(&now, NULL);
		timersub(&now, &last_sent, &timer_result);
		
		//printf("%d\n",(int) timer_result.tv_usec/1000);
		if((transmit_packet==1) || ((timer_result.tv_usec/1000)>KEEPALIVE))
		{
			for(x=0;x<8;x++)
			{
				printf("%d:%d, ", x, (packet_data[2*x+10]<<8)+packet_data[2*x+9]);
			}
			
			printf("\n");
			
			gettimeofday(&now, NULL);
			timersub(&now, &last_sent, &timer_result);
			
			if(timer_result.tv_usec>MAX_RATE && file_exists==0)
			{
				//ensure the receiver receives the latest packet by sending it twice
				for(x=0;x<2;x++)
				{
					
					pb_transmit_packet(ppcap, pcnt, packet_transmit_buffer, packet_header_length, packet_data, PACKET_LENGTH);
					pcnt++;
				}
			}
			
			gettimeofday(&last_sent, NULL);
		}
		else
		{
			usleep(1000);
		}
		
		for(x=0;x<PACKET_LENGTH;x++)
		{
			old_packet_data[x]=packet_data[x];
		}
	}

	close( joy_fd );	/* too bad we never get here */
	return 0;
}