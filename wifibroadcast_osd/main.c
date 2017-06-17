/*
Copyright (c) 2015, befinitiv
Copyright (c) 2012, Broadcom Europe Ltd
modified by Samuel Brucksch https://github.com/SamuelBrucksch/wifibroadcast_osd

All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
* Neither the name of the copyright holder nor the
names of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CA

#include <stdio.h>ND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// #define DEBUG
#include "osdconfig.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include "telemetry.h"
#ifdef FRSKY
#include "frsky.h"
#elif defined(LTM)
#include "ltm.h"
#elif defined(MAVLINK)
#include "mavlink.h"
#endif
#include "render.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>

long long current_timestamp() {
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // caculate milliseconds
    return milliseconds;
}

fd_set set;

struct timeval timeout;

int main(int argc, char *argv[]) {
	uint8_t buf[100];
	size_t n;

//	int width, height = 0;
	int loopcount = 0;
	printf("OSD started\n=====================================\n\n");

#ifdef FRSKY
	frsky_state_t fs;
#endif
	telemetry_data_t td;
	telemetry_init(&td);

	render_init();

		

	while(1) {
		FD_ZERO(&set);
		FD_SET(STDIN_FILENO, &set);
		timeout.tv_sec = 0;
		timeout.tv_usec = 100 * 1000;
		// look for data 100ms, then timeout

		Start(width,height);
		render_rssi(&td);

		n = select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout);
		if(n > 0) { // if data there, read it, parse it and render it
			n = read(STDIN_FILENO, buf, sizeof(buf));
//			printf("n read: %d\n",n);
			if(n == 0) {
				continue; // EOF
			}
			if(n<0) {
				perror("read");
				exit(-1);
			}
#ifdef FRSKY
			frsky_parse_buffer(&fs, &td, buf, n);
#elif defined(LTM)
			ltm_read(&td, buf, n);
#elif defined(MAVLINK)
			mavlink_read(&td, buf, n);
#endif
		}
		// start rendering
		render(&td);
		End(); // end rendering
	}
	return 0;
}
