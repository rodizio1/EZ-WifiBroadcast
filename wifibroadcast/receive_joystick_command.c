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

/* implements multiwii protocol on raspberry pi with standart joystick linux driver 
send also telemetry (if any) and send it to wifi */ 

#include <stdio.h>
#include <unistd.h>			//Used for UART
#include <fcntl.h>			//Used for UART
#include <termios.h>		//Used for UART

#include "rx_lib.h"

#define SERIAL_DEVICE "/dev/ttyAMA0"
#define PACKET_LENGTH 255
#define BAUD_RATE B115200

int uart0_filestream = -1;

int param_port = 2;
int param_data_packets_per_block = 1;
int param_fec_packets_per_block = 0;
int param_block_buffers = 1;
int param_packet_length = 22;
wifibroadcast_rx_status_t *rx_status = NULL;


int main(int argc, char *argv[])
{
	monitor_interface_t interfaces[MAX_PENUMBRA_INTERFACES];
	int num_interfaces = 0;
	
	block_buffer_t *block_buffer_list;
	int i;
	
	uart0_filestream = open(SERIAL_DEVICE, O_WRONLY | O_NOCTTY | O_NDELAY);	
	
	if (uart0_filestream == -1)
	{
		//ERROR - CAN'T OPEN SERIAL PORT
		printf("Error - Unable to open UART.  Ensure it is not in use by another application\n");
	}
	
	if (argc < 2 || argc > 3 || !strcmp("--help", argv[1])) {
		puts("");
		puts("Usage: joystick_receive <device>");
		puts("");
		exit(1);
	}
	
	//CONFIGURE THE UART
	//The flags (defined in /usr/include/termios.h - see http://pubs.opengroup.org/onlinepubs/007908799/xsh/termios.h.html):
	//	Baud rate:- B1200, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800, B500000, B576000, B921600, B1000000, B1152000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000
	//	CSIZE:- CS5, CS6, CS7, CS8
	//	CLOCAL - Ignore modem status lines
	//	CREAD - Enable receiver
	//	IGNPAR = Ignore characters with parity errors
	//	ICRNL - Map CR to NL on input (Use for ASCII comms where you want to auto correct end of line characters - don't use for bianry comms!)
	//	PARENB - Parity enable
	//	PARODD - Odd parity (else even)
	struct termios options;
	tcgetattr(uart0_filestream, &options);
	
	cfmakeraw(&options);
	
	//set baudrate
	cfsetispeed(&options, BAUD_RATE);
	cfsetospeed(&options, BAUD_RATE);
	
	//enable receiving
	options.c_cflag |=CREAD;

	//minimum number of chars to read
	options.c_cc[VMIN]=22;
	//timeout reading
	options.c_cc[VTIME]=2;
	//no flowcontrol
	//options.c_cflag &= ~CNEW_RTSCTS;
	
	//write options
	tcsetattr(uart0_filestream, TCSANOW, &options);
	
	
	fec_init();
	int x = optind;
	while(x < argc && num_interfaces < MAX_PENUMBRA_INTERFACES) {
		open_and_configure_interface(argv[x], param_port, interfaces + num_interfaces);
		++num_interfaces;
		++x;
	}
	
	//block buffers contain both the block_num as well as packet buffers for a block.
    block_buffer_list = malloc(sizeof(block_buffer_t) * param_block_buffers);
    for(i=0; i<param_block_buffers; ++i)
	{
        block_buffer_list[i].block_num = -1;
        block_buffer_list[i].packet_buffer_list = lib_alloc_packet_buffer_list(param_data_packets_per_block+param_fec_packets_per_block, MAX_PACKET_LENGTH);
	}
	
	rx_status = status_memory_open(param_port);
	rx_status->wifi_adapter_cnt = num_interfaces;
	
	while(1)
	{
		process_packet(interfaces, block_buffer_list, i, param_data_packets_per_block, param_fec_packets_per_block, param_packet_length, rx_status, param_block_buffers,1,uart0_filestream);
		usleep(1000);
	}
	
	close(uart0_filestream);
}