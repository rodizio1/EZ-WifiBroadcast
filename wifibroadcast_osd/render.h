#pragma once

#include "bcm_host.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "VG/openvg.h"
#include "VG/vgu.h"
#include "fontinfo.h"
#include "shapes.h"
#include <math.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <stdio.h>
#include "telemetry.h"

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
#include <stdbool.h>

#define TO_DEG 180.0f / M_PI

void render_init();
void render(telemetry_data_t *td);
void render_rssi(telemetry_data_t *td);
//void render_rollpitch(telemetry_data_t *td);
//void render_heading(telemetry_data_t *td);
//void render_batt(telemetry_data_t *td);

//rotate a polyline/polygon with this
void rotatePoints(float *x, float *y, int angle, int points, int center_x, int center_y);

void paintArrow(int heading, int pos_x, int pos_y);
void paintAHI(int hor_angle, int ver_angle);
void draw_total_signal(int8_t signal, int goodblocks, int badblocks, int lostpackets, int packets_received, int data_received, int valid_msgs,int pos_x, int pos_y, float scale, int color);
void draw_rc_signal(int8_t signal, int lostpackets,  int pos_x, int pos_y, float scale, int color);
void draw_card_signal(int8_t signal, int card, int packets, int wrongcrcs, int type, int pos_x, int pos_y, float scale);
float distance_between(float lat1, float long1, float lat2, float long2);
float course_to (float lat1, float long1, float lat2, float long2);

//new stuff from fritz walter https://www.youtube.com/watch?v=EQ01b3aJ-rk
//this will only indicate how much % are left. Mavlink specific, but could be used with others as well.
void draw_bat_remaining(int remaining, int pos_x, int pos_y, float scale);
void draw_compass(int heading, int pos_x, int pos_y, bool ladder_enabled, float scale);
void draw_bat_status(float voltage, float current, int pos_x, int pos_y, float scale);
void draw_batt_mah(float voltage, float current, float pos_x, float pos_y, float scale);
void draw_position(float lat, float lon, bool fix, int sats, int fixtype, int pos_x, int pos_y, float scale);
void draw_home_distance(int distance, int pos_x, int pos_y, float scale);
//autopilot mode, mavlink specific, could be used if mode is in telemetry data of other protocols as well
void draw_mode(char *mode, int pos_x, int pos_y, float scale);
void draw_home_indicator(int home_angle, int pos_x, int pos_y, float scale);
void draw_altitude(int alt, int pos_x, int pos_y, bool ladder_enabled, float scale);
void draw_speed(int speed, int pos_x, int pos_y, bool ladder_enabled, float scale);
//ladder here means the additional lines of the AHI, if true all lines will be drawn, if false only the main line
void draw_horizon(float roll, float pitch, int pos_x, int pos_y, float scale);

int width, height; // needed to be able to call Start() and End() in main.c
