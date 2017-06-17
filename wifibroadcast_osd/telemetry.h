#pragma once

#include <stdint.h>
#include <time.h>
#include "osdconfig.h"

typedef struct {
        uint32_t received_packet_cnt;
        uint32_t wrong_crc_cnt;
        int8_t current_signal_dbm;
	int8_t type;
} wifi_adapter_rx_status_t;

typedef struct {
        uint32_t received_packet_cnt;
        uint32_t wrong_crc_cnt;
        int8_t current_signal_dbm;
	int8_t type;
} wifi_adapter_rx_status_t_osd;

typedef struct {
        uint32_t received_packet_cnt;
        uint32_t wrong_crc_cnt;
        int8_t current_signal_dbm;
	int8_t type;
} wifi_adapter_rx_status_t_rc;

typedef struct {
        time_t last_update;
        uint32_t received_block_cnt;
        uint32_t damaged_block_cnt;
	uint32_t lost_packet_cnt;
	uint32_t received_packet_cnt;
        uint32_t tx_restart_cnt;

        uint32_t wifi_adapter_cnt;
        wifi_adapter_rx_status_t adapter[8];
} wifibroadcast_rx_status_t;

typedef struct {
        time_t last_update;
        uint32_t received_block_cnt;
        uint32_t damaged_block_cnt;
	uint32_t lost_packet_cnt;
	uint32_t received_packet_cnt;
        uint32_t tx_restart_cnt;

        uint32_t wifi_adapter_cnt;
        wifi_adapter_rx_status_t adapter[8];
} wifibroadcast_rx_status_t_osd;

typedef struct {
        time_t last_update;
        uint32_t received_block_cnt;
        uint32_t damaged_block_cnt;
	uint32_t lost_packet_cnt;
	uint32_t received_packet_cnt;
        uint32_t tx_restart_cnt;

        uint32_t wifi_adapter_cnt;
        wifi_adapter_rx_status_t adapter[8];
} wifibroadcast_rx_status_t_rc;




typedef struct {
	uint32_t validmsgsrx;
	uint32_t datarx;
	float voltage;
	float ampere;
	float baro_altitude;
	float altitude;
	double longitude;
	double latitude;
	float heading;
	float speed;
	float airspeed;
	int16_t x, y, z;
	int16_t ew, ns;

	uint8_t sats;
	uint8_t fix;

#if defined(LTM) || defined(MAVLINK)
	int16_t roll, pitch;
	uint8_t rssi;
//	float hdop;
#endif

#if defined LTM
// ltm S frame
	uint8_t status;
	uint8_t uav_arm;
	uint8_t uav_failsafe;
	uint8_t uav_flightmode;
// ltm N frame
	uint8_t gpsmode;
	uint8_t navmode;
	uint8_t navaction;
	uint8_t wpnumber;
	uint8_t naverror;
// ltm x frame
	uint8_t hw_status;
// ltm o frame
	float home_altitude;
	double home_longitude;
	double home_latitude;
	uint8_t osdon;
	uint8_t home_fix;
#endif


	wifibroadcast_rx_status_t *rx_status;
	wifibroadcast_rx_status_t_osd *rx_status_osd;
	wifibroadcast_rx_status_t_rc *rx_status_rc;
} telemetry_data_t;

wifibroadcast_rx_status_t *telemetry_wbc_status_memory_open(void);
wifibroadcast_rx_status_t_osd *telemetry_wbc_status_memory_open_osd(void);
wifibroadcast_rx_status_t_rc *telemetry_wbc_status_memory_open_rc(void);
