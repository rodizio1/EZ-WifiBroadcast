// (c)2016 Philippe Crochat - Anemos Technologies (pcrochat@anemos-technologies.com) based on befinitiv wifibroadcast

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
 */

#include "rx_lib.h"
#include "tx_lib.h"
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <linux/joystick.h>

/* This assumes you have the mavlink headers on your include path
 or in the same folder as this source file */
//#include "mavlink/includes/common/mavlink.h"

#define PORT 5000
#define PORT_TELEMETRY 5001
#define PORT_RSSI 5003
#define BUFFER_LENGTH 40
#define MAX_TELEMETRY_LENGTH 32


/* joystick specific code*/
#define JOY_DEV "/dev/input/js0"
#define JOY_NAME_LENGTH 128
#define KEEPALIVE 100 //in milliseconds, when there is more than KEEPALIVE ms elapsed we resend the packet even if it is the same
#define JOY_PACKET_LENGTH 26
#define JOY_MAX_RATE 100000 //max 1 packet every MAX_RATE microseconds

//reattribute those channels based on your needs
int CHANNEL_INDEX[8]={1, 4, 3, 0, 2, 5, 6, 7};

int channel_values[8];
int old_channel_values[8];

uint8_t telemetry_buffer_to_send[BUFFER_LENGTH];

int param_port = 0;
int param_data_packets_per_block = 8;
int param_fec_packets_per_block = 4;
int param_block_buffers = 1;
int param_packet_length = MAX_USER_PACKET_LENGTH;
int flagHelp = 0;
char *param_ip_address;
wifibroadcast_rx_status_t *rx_status = NULL;
struct sockaddr_in si_other,si_other_telemetry,si_other_rssi;
int s, s_telemetry, s_rssi, slen=sizeof(si_other), slen_telemetry=sizeof(si_other_telemetry), slen_rssi=sizeof(si_other_rssi);
//mavlink_message_t msg;
uint16_t len;
uint8_t buf[1024];
int buffer_index=0;
uint64_t micros=0;
ssize_t recsize;
socklen_t fromlen;
struct sockaddr_in locAddr;
uint64_t microsSinceEpoch();

/* 
* error - wrapper for perror
*/
void diep(char *s)
{
	perror(s);
	exit(1);
}

/* mavlink for future use
void send_parameter_list(void)
{
	mavlink_msg_param_value_pack(1, 1, &msg, "SR0_EXT_STAT", 5, 4, 711, 65535);
	len = mavlink_msg_to_send_buffer(buf, &msg);
	if(sendto(s_telemetry, buf, len, 0, (struct sockaddr*)&si_other_telemetry, slen_telemetry)==-1)
		diep("sendto() parameter");
}

void send_mission_count(void)
{
	mavlink_msg_mission_count_pack(1, 1, &msg, 255, 190, 0);
	len = mavlink_msg_to_send_buffer(buf, &msg);
	if(sendto(s_telemetry, buf, len, 0, (struct sockaddr*)&si_other_telemetry, slen_telemetry)==-1)
		diep("sendto() mission count");
}
*/

void usage(void)
{
	printf(
	    "(c)2015 befinitiv. Based on packetspammer by Andy Green.  Licensed under GPL2\n"
	    "\n"
	    "Usage: rx [options] <interfaces>\n\nOptions\n"
	    "-p <port>   Port number 0-255 (default 0)\n"
	    "-b <count>  Number of data packets in a block (default 8). Needs to match with tx.\n"
	    "-r <count>  Number of FEC packets per block (default 4). Needs to match with tx.\n"
	    "-f <bytes>  Number of bytes per packet (default %d. max %d). This is also the FEC block size. Needs to match with tx\n"
	    "-d <blocks> Number of transmissions blocks that are buffered (default 1). This is needed in case of diversity if one\n"
	    "            adapter delivers data faster than the other. Note that this increases latency\n"
	    "\n"
	    "Example:\n"
	    "  rx -b 8 -r 4 -f 1024 wlan0 | cat /dev/null  (receive raw packets on wlan0 and throw away payload to /dev/null)\n"
	    "\n", MAX_USER_PACKET_LENGTH, MAX_USER_PACKET_LENGTH);
	exit(1);
}


void treatment(uint8_t *buffer, int buffer_length)
{
	int telemetry_size=*(buffer);
	//int i;
	int remaining=0;
	//unsigned int temp = 0;
	
	if(buffer_length-1-telemetry_size>0)
	{
		write(STDOUT_FILENO, buffer+1, buffer_length-1-telemetry_size);
		//fprintf(stderr, "%d bytes video written\n", buffer_length-1-telemetry_size);
	}
	
	//mean transmission error, better not sending it
	if(telemetry_size>MAX_TELEMETRY_LENGTH)
		telemetry_size=0;
	
	//send telemetry
	if(telemetry_size>0)
	{
		fprintf(stderr, "telemetry %d\n", telemetry_size);
		/* for futur use mavlink
		
		if(sendto(s_telemetry, buffer+buffer_length-telemetry_size, telemetry_size, 0, (struct sockaddr*)&si_other_telemetry, slen_telemetry)==-1)
			diep("sendto()");
		
		
		if(micros==0 || microsSinceEpoch()-micros>500000)
		{
			mavlink_msg_heartbeat_pack(1, 200, &msg, MAV_TYPE_HELICOPTER, MAV_AUTOPILOT_GENERIC, MAV_MODE_GUIDED_ARMED, 0, MAV_STATE_ACTIVE);
			len = mavlink_msg_to_send_buffer(buf, &msg);
			
			printf("heartbeat sent\n");
			
			if(sendto(s_telemetry, buf, len, 0, (struct sockaddr*)&si_other_telemetry, slen_telemetry)==-1)
				diep("sendto() heartbeat");
			
			
			mavlink_msg_sys_status_pack(1, 200, &msg, 0, 0, 0, 500, 11000, -1, -1, 0, 0, 0, 0, 0, 0);
			len = mavlink_msg_to_send_buffer(buf, &msg);
			
			if(sendto(s_telemetry, buf, len, 0, (struct sockaddr*)&si_other_telemetry, slen_telemetry)==-1)
				diep("sendto() heartbeat");
			
			micros=microsSinceEpoch();
		}
		
		memset(buf, 0, 1024);
		
		recsize = recvfrom(s_telemetry, (void *)buf, 1024, 0, (struct sockaddr *)&si_other_telemetry, &fromlen);
		if (recsize > 0)
      	{
			// Something received - print out all bytes and parse packet
			mavlink_message_t msg;
			mavlink_status_t status;
			
			printf("Bytes Received: %d\nDatagram: ", (int)recsize);
			for (i = 0; i < recsize; ++i)
			{
				temp = buf[i];
				printf("%02x ", (unsigned char)temp);
				if (mavlink_parse_char(MAVLINK_COMM_0, buf[i], &msg, &status))
				{
					// Packet received
					printf("\nReceived packet: SYS: %d, COMP: %d, LEN: %d, MSG ID: %d\n", msg.sysid, msg.compid, msg.len, msg.msgid);
					
					//param request list
					if(msg.msgid==0x15)
						send_parameter_list();
					
					//mission request list
					if(msg.msgid==0x2b)
						send_mission_count();
						
				}
			}
			printf("\n");
		}
		memset(buf, 0, 1024);
		*/
		
		if(buffer_index+telemetry_size>=BUFFER_LENGTH)
		{
			remaining=(buffer_index+telemetry_size)%BUFFER_LENGTH;
			memcpy(telemetry_buffer_to_send+buffer_index, buffer+buffer_length-telemetry_size, telemetry_size-remaining);
		
			if (sendto(s_telemetry, telemetry_buffer_to_send, BUFFER_LENGTH, 0, (struct sockaddr*)&si_other_telemetry, slen_telemetry)==-1)
				diep("sendto()");
			
			buffer_index=0;
		}
		else
		{
			remaining=telemetry_size;
		}
		
		memcpy(telemetry_buffer_to_send+buffer_index, buffer+buffer_length-remaining, remaining);
		buffer_index+=remaining;
		
		if (sendto(s_telemetry, buffer+buffer_length-telemetry_size, telemetry_size, 0, (struct sockaddr*)&si_other_telemetry, slen_telemetry)==-1)
			diep("sendto()");
	}
}

int main(int argc, char *argv[])
{
	monitor_interface_t interfaces[MAX_PENUMBRA_INTERFACES];
	int num_interfaces = 0;
	int i;
	int joy_fd;
	int *axis=NULL;
	unsigned char num_of_axis = 2;
	unsigned char num_of_buttons = 2;
	int version = 0x000800;
	char name_of_joystick[JOY_NAME_LENGTH] = "Unknown";
    block_buffer_t *block_buffer_list;
    uint8_t rssi;
    struct timeval joystick_last_sent;
    struct timeval now;
    struct timeval timer_result;
    uint8_t packet_transmit_buffer[MAX_PACKET_LENGTH];
    size_t joystick_packet_header_length = 0;
    uint8_t packet_data[JOY_PACKET_LENGTH];
	uint8_t old_packet_data[JOY_PACKET_LENGTH];
	uint8_t crc;
    int pcnt = 0;
    char *buttons=NULL;
    int file_exists;
    char szErrbuf[PCAP_ERRBUF_SIZE];
	pcap_t *ppcap_joystick = NULL;
    
	while (1) {
		int nOptionIndex;
		static const struct option optiona[] = {
			{ "help", no_argument, &flagHelp, 1 },
			{ 0, 0, 0, 0 }
		};
		int c = getopt_long(argc, argv, "a:hp:b:d:r:f:",
			optiona, &nOptionIndex);

		if (c == -1)
			break;
		switch (c) {
		case 0: // long option
			break;

		case 'h': // help
			usage();

		case 'a': //ip address
			param_ip_address=optarg;
			printf("ip %s", param_ip_address);
			break;
			
		case 'p': //port
			param_port = atoi(optarg);
			break;
		
		case 'b': 
			param_data_packets_per_block = atoi(optarg);
			break;

		case 'r': 
			param_fec_packets_per_block = atoi(optarg);
			break;
		
		case 'd':
            param_block_buffers = atoi(optarg);
			break;
		
		case 'f': // MTU
			param_packet_length = atoi(optarg);
			break;

		default:
			fprintf(stderr, "unknown switch %c\n", c);
			usage();
			break;
		}
	}

	if ((joy_fd = open(JOY_DEV, O_RDONLY)) < 0) {
		perror("joystick");
		exit(1);
	}
	
	gettimeofday(&joystick_last_sent, NULL);
	
	ioctl(joy_fd, JSIOCGVERSION, &version);
	ioctl(joy_fd, JSIOCGAXES, &num_of_axis);
	ioctl(joy_fd, JSIOCGBUTTONS, &num_of_buttons);
	ioctl(joy_fd, JSIOCGNAME(JOY_NAME_LENGTH), name_of_joystick);
	
	printf("Joystick (%s) has %d axes and %d buttons. Driver version is %d.%d.%d.\n",
	name_of_joystick, num_of_axis, num_of_buttons, version >> 16, (version >> 8) & 0xff, version & 0xff);
	
	axis = (int32_t *) calloc( num_of_axis, sizeof( int32_t ) );
	buttons = (char *) calloc( num_of_buttons+1, sizeof( char ) );
	
	struct js_event js;
	fcntl( joy_fd, F_SETFL, O_NONBLOCK );	/* use joystick non-blocking mode */
	
	joystick_packet_header_length = packet_header_init(packet_transmit_buffer, 18, 0);
	
	if (optind >= argc)
		usage();
	
	
	if(param_packet_length > MAX_USER_PACKET_LENGTH) {
		printf("Packet length is limited to %d bytes (you requested %d bytes)\n", MAX_USER_PACKET_LENGTH, param_packet_length);
		return (1);
	}

	if ((s=socket(PF_INET, SOCK_DGRAM, 0))==-1)
    	diep("socket");
    
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(PORT);
    si_other.sin_addr.s_addr = inet_addr(param_ip_address);
    memset(si_other.sin_zero, '\0', sizeof(si_other.sin_zero));

    if ((s_telemetry=socket(PF_INET, SOCK_DGRAM, 0))==-1)
    	diep("socket");
    
    if ((s_rssi=socket(PF_INET, SOCK_DGRAM, 0))==-1)
    	diep("socket");
    
    memset(&locAddr, 0, sizeof(locAddr));
	locAddr.sin_family = AF_INET;
	locAddr.sin_addr.s_addr = INADDR_ANY;
	locAddr.sin_port = htons(14550);
    
	/* for future use telemetry message injection
	if (-1 == bind(s_telemetry,(struct sockaddr *)&locAddr, sizeof(struct sockaddr)))
    {
		perror("error bind failed");
		close(s_telemetry);
		exit(EXIT_FAILURE);
    } 
	
	Attempt to make it non blocking 
	if (fcntl(s_telemetry, F_SETFL, O_NONBLOCK | FASYNC) < 0)
    {
		fprintf(stderr, "error setting nonblocking: %s\n", strerror(errno));
		close(s_telemetry);
		exit(EXIT_FAILURE);
    }*/
    
    si_other_telemetry.sin_family = AF_INET;
    si_other_telemetry.sin_port = htons(PORT_TELEMETRY);
    si_other_telemetry.sin_addr.s_addr = inet_addr(param_ip_address);
    memset(si_other_telemetry.sin_zero, '\0', sizeof(si_other_telemetry.sin_zero));
    
    si_other_rssi.sin_family = AF_INET;
    si_other_rssi.sin_port = htons(PORT_RSSI);
    si_other_rssi.sin_addr.s_addr = inet_addr(param_ip_address);
    memset(si_other_rssi.sin_zero, '\0', sizeof(si_other_rssi.sin_zero));
    
	fec_init();

	int x = optind;
	while(x < argc && num_interfaces < MAX_PENUMBRA_INTERFACES) {
		open_and_configure_interface(argv[x], interfaces + num_interfaces);
		++num_interfaces;
		++x;
	}

	szErrbuf[0] = '\0';
	ppcap_joystick = pcap_open_live(argv[optind], 800, 1, 20, szErrbuf);
	if (ppcap_joystick == NULL) {
		fprintf(stderr, "Unable to open interface %s in pcap: %s\n",
		    argv[optind], szErrbuf);
		return (1);
	}

	pcap_setnonblock(ppcap_joystick, 0, szErrbuf);

    //block buffers contain both the block_num as well as packet buffers for a block.
    block_buffer_list = malloc(sizeof(block_buffer_t) * param_block_buffers);
    for(i=0; i<param_block_buffers; ++i)
	{
        block_buffer_list[i].block_num = -1;
        block_buffer_list[i].packet_buffer_list = lib_alloc_packet_buffer_list(param_data_packets_per_block+param_fec_packets_per_block, MAX_PACKET_LENGTH);
	}


	rx_status = status_memory_open(param_port);
	rx_status->wifi_adapter_cnt = num_interfaces;

	
	for(;;) { 
		fd_set readset;
		struct timeval to;

		to.tv_sec = 0;
		to.tv_usec = 1e5;
	
		FD_ZERO(&readset);
		for(i=0; i<num_interfaces; ++i)
		{
			//fprintf(stderr,"device %d\n", interfaces[i].selectable_fd);
			FD_SET(interfaces[i].selectable_fd, &readset);
		}

		int n = select(30, &readset, NULL, NULL, &to);

		file_exists=access( JOY_DEV, F_OK ); //test if joystick is still connected
		
		for(i=0; i<num_interfaces; ++i) {
			if(n == 0)
				break;

			if(FD_ISSET(interfaces[i].selectable_fd, &readset)) {
				process_packet(interfaces + i, block_buffer_list, i, param_data_packets_per_block, param_fec_packets_per_block, param_packet_length, rx_status, param_block_buffers, &treatment);
				
				/* read the joystick state */
				if (read(joy_fd, &js, sizeof(struct js_event)) != sizeof(struct js_event)) {
					//usleep(100);
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
					
					channel_values[x]=(packet_data[2*x+10]<<8)+packet_data[2*x+9];
					
					if(x==2)
					{
						if(channel_values[2]==1000 && old_channel_values[2]>1000)
						{
							packet_data[13]=(uint8_t)(old_channel_values[2]&0xFF);
							packet_data[14]=(uint8_t)(old_channel_values[2]>>8);
						}
					}
					
					old_channel_values[x]=channel_values[x];
				}
				
				//build crc 
				crc=0;
				for(x=7;x<25;x++)
				{
					crc^=packet_data[x];
				}
				
				packet_data[25]=crc;
				
				int transmit_packet=0;
				
				for(x=0;x<JOY_PACKET_LENGTH;x++)
				{
					if(packet_data[x]!=old_packet_data[x])
					{
						//fprintf(stderr, "%d:%d:%d\n", x, packet_data[x], old_packet_data[x]);
						transmit_packet=1;
						break;
					}
				}
				
				gettimeofday(&now, NULL);
				timersub(&now, &joystick_last_sent, &timer_result);
				
				//printf("%d\n",(int) timer_result.tv_usec/1000);
				if((transmit_packet==1) || ((timer_result.tv_usec/1000)>KEEPALIVE))
				{
					payload_header_t *ph = (payload_header_t*)packet_data;
					ph->data_length = 22; //MSP packet length
					
					gettimeofday(&now, NULL);
					timersub(&now, &joystick_last_sent, &timer_result);
					
					//fprintf(stderr, "timer : %d, %d\n", (int)timer_result.tv_usec, JOY_MAX_RATE);
					if(timer_result.tv_usec>JOY_MAX_RATE && file_exists==0)
					{
						for(x=0;x<8;x++)
						{
							fprintf(stderr, "%d:%d, ", x, channel_values[x]);
						}
						
						fprintf(stderr, "\n");
						
						//ensure the receiver receives the latest packet by sending it twice
						for(x=0;x<2;x++)
						{
							fprintf(stderr, "transmit joystick_values %d\n",pcnt);
							pb_transmit_packet(ppcap_joystick, pcnt, packet_transmit_buffer, joystick_packet_header_length, packet_data, JOY_PACKET_LENGTH);
							//usleep(200);
							pcnt++;
						}
						
						gettimeofday(&joystick_last_sent, NULL);
					}
					
				}
				else
				{
					//usleep(1000);
				}
				
				for(x=0;x<JOY_PACKET_LENGTH;x++)
				{
					old_packet_data[x]=packet_data[x];
				}
			}
		}
		
		//send rssi value
		rssi=-rx_status->adapter[0].current_signal_dbm;
				
		
		//fprintf(stderr, "rssi %d\n", rx_status->adapter[0].current_signal_dbm);
		if (sendto(s_rssi, &rssi, 1, 0, (struct sockaddr*)&si_other_rssi, slen_rssi)==-1)
			diep("sendto()");
		
		
	}

	return (0);
}

/* QNX timer version */
#if (defined __QNX__) | (defined __QNXNTO__)
uint64_t microsSinceEpoch()
{
	
	struct timespec time;
	
	uint64_t micros = 0;
	
	clock_gettime(CLOCK_REALTIME, &time);  
	micros = (uint64_t)time.tv_sec * 1000000 + time.tv_nsec/1000;
	
	return micros;
}
#else
uint64_t microsSinceEpoch()
{
	
	struct timeval tv;
	
	uint64_t micros = 0;
	
	gettimeofday(&tv, NULL);  
	micros =  ((uint64_t)tv.tv_sec) * 1000000 + tv.tv_usec;
	
	return micros;
}
#endif