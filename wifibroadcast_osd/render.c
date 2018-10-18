#include <stdint.h>
#include "render.h"
#include "telemetry.h"
#include "osdconfig.h"

int width, height;
#define TO_FEET 3.28084
#define TO_MPH 0.621371
#define CELL_WARNING_PCT1 (CELL_WARNING1-CELL_MIN)/(CELL_MAX-CELL_MIN)*100
#define CELL_WARNING_PCT2 (CELL_WARNING2-CELL_MIN)/(CELL_MAX-CELL_MIN)*100

bool setting_home;
bool home_set;
float home_lat;
float home_lon;
int home_counter;
char buffer[50];
float mAhDrawRaw, mAhDrawn;
bool first = true;
long long lastT, timeDiff;

long long current_ts() {
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
    return milliseconds;
}

int getWidth(float pos_x_percent) {
	return (width * 0.01f * pos_x_percent);
}

int getHeight(float pos_y_percent) {
	return (height * 0.01f * pos_y_percent);
}

int scale_factor;

void render_init() {
        init(&width, &height);
	scale_factor = width/170 * FONTSIZE;
	home_counter = 0;
//        vgSeti(VG_MATRIX_MODE, VG_MATRIX_GLYPH_USER_TO_SURFACE);
}



void render(telemetry_data_t *td) {

#ifdef ALT
//	if (home_set){
		#ifdef IMPERIAL

		#if USEBAROALT == true
		draw_altitude((int)(td->baro_altitude * TO_FEET), getWidth(60), getHeight(50), DRAW_ALT_LADDER, 2);
		#else
		draw_altitude((int)(td->altitude * TO_FEET), getWidth(60), getHeight(50), DRAW_ALT_LADDER, 2);
		#endif

		#else

		#if USEBAROALT == true
		draw_altitude((int)td->baro_altitude, getWidth(60), getHeight(50), DRAW_ALT_LADDER, 2);
		#else
		draw_altitude((int)td->altitude, getWidth(60), getHeight(50), DRAW_ALT_LADDER, 2);
		#endif

		#endif
//	}
#endif

#ifdef SPEED
//	if (home_set){
		#ifdef IMPERIAL

		#if USEAIRSPEED == true
		draw_speed((int)td->airspeed*TO_MPH, getWidth(40), getHeight(50), DRAW_SPEED_LADDER, 2);
		#else
		draw_speed((int)td->speed*TO_MPH, getWidth(40), getHeight(50), DRAW_SPEED_LADDER, 2);
		#endif

		#else

		#if USEAIRSPEED == true
		draw_speed((int)td->airspeed, getWidth(40), getHeight(50), DRAW_SPEED_LADDER, 2);
		#else
		draw_speed((int)td->speed, getWidth(40), getHeight(50), DRAW_SPEED_LADDER, 2);
		#endif

		#endif
//	}
#endif

#ifdef HOME_ARROW
	if (home_set)
		#ifdef FRSKY
		paintArrow((int)course_to((td->ns == 'N'? 1:-1) *td->latitude, (td->ns == 'E'? 1:-1) *td->longitude, home_lat, home_lon), getWidth(50), getHeight(80));
		#else
		paintArrow((int)course_to(home_lat, home_lon, td->latitude, td->longitude), getWidth(50), getHeight(80));
		#endif
#endif

#ifdef HEADING
	draw_compass(td->heading, getWidth(50), getHeight(86), DRAW_COURSE_LADDER, 1.5);
#endif


#ifdef BATT_STATUS
	draw_bat_status(td->voltage, td->ampere, getWidth(20), getHeight(5), scale_factor * 3);
#endif

#ifdef BATT_MAH
    draw_batt_mah(td->voltage, td->ampere, BATT_MAH_POS_X, BATT_MAH_POS_Y, BATT_MAH_SCALE * GLOBAL_SCALE);
#endif
	
#ifdef BATT_REMAINING
	draw_bat_remaining(((td->voltage/CELLS)-CELL_MIN)/(CELL_MAX-CELL_MIN)*100, getWidth(5), getHeight(5), 3);
#endif

#ifdef POSITION
	#if defined(FRSKY)
	//we assume that if we get the NS and EW values from frsky protocol, that we have a fix
	if ((td->ew == 'E' || td->ew == 'W') && (td->ns == 'N' || td->ns == 'S')){
		setting_home = true;
		draw_position((td->ns == 'N'? 1:-1) * td->latitude, (td->ew == 'E'? 1:-1) * td->longitude, true, 0, 0, getWidth(90), getHeight(5), scale_factor*2.7);
	}else{
		//no fix
		setting_home = false;
		home_counter = 0;
		draw_position((td->ns == 'N'? 1:-1) * td->latitude, (td->ew == 'E'? 1:-1) * td->longitude, false, 0, 0, getWidth(80), getHeight(5), scale_factor*2.7);
	}
	
	//if 20 packages after each other have a fix automatically set home
	if (setting_home && !home_set){
		if (++home_counter == 20){
			home_set = true;
			home_lat = (td->ns == 'N'? 1:-1) * td->latitude;
			home_lon = (td->ew == 'E'? 1:-1) * td->longitude;
		}
	}
	#endif

	#if defined(MAVLINK)
	// if satfix is reported by flightcontrol
	if (td->fix > 2 && !home_set){
		setting_home = true;
	}

	//if 20 packages after each other have a fix, automatically set home
	if (setting_home && !home_set){
		if (++home_counter == 20){
			home_set = true;
			home_lat = td->latitude;
			home_lon = td->longitude;
			td->ns = 1;
			td->ew = 1;
		}
	}
	draw_position(td->latitude, td->longitude, td->fix, td->sats, td->fix, getWidth(80), getHeight(5), scale_factor*2.5);
	#endif

	#if defined(LTM)
	// if satfix is reported by flightcontrol
	if (td->fix > 2 && !home_set){
		setting_home = true;
	}

	if (setting_home && !home_set){
		//if 20 packages after each other have a fix
//		if (++home_counter == 20){
		    //if LTM O frame reports home fix, set home position, use lat/long from O frame
		    if (td->home_fix == 1){
			home_set = true;
			home_lat = td->home_latitude;
			home_lon = td->home_longitude;
			td->ns = 1;
			td->ew = 1;
		    }
//		}
	}
	draw_position(td->latitude, td->longitude, td->fix, td->sats, td->fix, getWidth(80), getHeight(5), scale_factor*2.5);
	#endif
#endif



#ifdef DISTANCE
	if (home_set)
		#ifdef FRSKY
		draw_home_distance((int)distance_between(home_lat, home_lon, (td->ns == 'N'? 1:-1) *td->latitude, (td->ns == 'E'? 1:-1) *td->longitude), getWidth(50), getHeight(5), scale_factor * 2.5);
		#elif defined(LTM) || defined(MAVLINK)
                draw_home_distance((int)distance_between(home_lat, home_lon, td->latitude, td->longitude), getWidth(50), getHeight(5), scale_factor * 2.5);
		#endif
#endif

#ifdef HORIZON
#if defined(FRSKY)
	float x_val, y_val, z_val;
    	x_val = td->x;
    	y_val = td->y;
    	z_val = td->z;

	#ifdef EXCHANGE_ROLL_AND_PITCH
		#if DRAW_AHI_LADDER == true
	draw_horizon(INVERT_ROLL * TO_DEG * (atan(y_val / sqrt((x_val*x_val) + (z_val*z_val)))), INVERT_PITCH * TO_DEG * (atan(x_val / sqrt((y_val*y_val)+(z_val*z_val)))), getWidth(50), getHeight(50), 1.5f);
		#else
	paintAHI(INVERT_ROLL * TO_DEG * (atan(y_val / sqrt((x_val*x_val) + (z_val*z_val)))), INVERT_PITCH * TO_DEG * (atan(x_val / sqrt((y_val*y_val)+(z_val*z_val)))));
		#endif //AHI ladder
	#else
		#if DRAW_AHI_LADDER == true
	draw_horizon((int)(INVERT_ROLL * TO_DEG * (atan(y_val / sqrt((x_val*x_val) + (z_val*z_val))))), (int)( INVERT_PITCH * TO_DEG * (atan(x_val / sqrt((y_val*y_val)+(z_val*z_val))))), getWidth(50), getHeight(50), 1.5f);
		#else
	paintAHI(INVERT_ROLL * TO_DEG * (atan(x_val / sqrt((y_val*y_val) + (z_val*z_val)))), INVERT_PITCH * TO_DEG * (atan(y_val / sqrt((x_val*x_val)+(z_val*z_val)))));
		#endif // AHI ladder
	#endif // EXCHANGE_ROLL_AND_PITCH
#elif defined(LTM) || defined(MAVLINK)
	#if DRAW_AHI_LADDER == true
	draw_horizon(INVERT_ROLL * td->roll, INVERT_PITCH * td->pitch, getWidth(50), getHeight(50), 1.5f);
	#else
	paintAHI(INVERT_ROLL * td->roll, INVERT_PITCH * td->pitch);
	#endif //AHI ladder
#endif //protocol
#endif //HORIZON



}


void render_rssi(telemetry_data_t *td) {
#ifdef VIDEO_RSSI
    if(td->rx_status != NULL) {
	int i;
	int ac = td->rx_status->wifi_adapter_cnt;
	int best_dbm = -1000;
	float dbm_scale = 2;

	// find out which card has best signal
	for(i=0; i<ac; ++i) {
	    if (best_dbm < td->rx_status->adapter[i].current_signal_dbm) best_dbm = td->rx_status->adapter[i].current_signal_dbm;
	}
	dbm_scale = 2.27778-0.0138889 * best_dbm;

//	draw_total_signal(best_dbm, td->rx_status->received_block_cnt, td->rx_status->damaged_block_cnt, td->rx_status->lost_packet_cnt, td->rx_status->received_packet_cnt, getWidth(6), getHeight(92), scale_factor * 1.5 * dbm_scale, 255);
	draw_total_signal(best_dbm, td->rx_status->received_block_cnt, td->rx_status->damaged_block_cnt, td->rx_status->lost_packet_cnt, td->rx_status->received_packet_cnt, td->datarx, td->validmsgsrx, getWidth(6), getHeight(92), scale_factor * 1.1 * dbm_scale, 255);
	#ifdef VIDEO_RSSI_DETAILED
	for(i=0; i<ac; ++i) {
//	    draw_card_signal(td->rx_status->adapter[i].current_signal_dbm, i, td->rx_status->adapter[i].received_packet_cnt, td->rx_status->adapter[i].wrong_crc_cnt, getWidth(6), getHeight(82 - i * 2), scale_factor*1.7);
	    draw_card_signal(td->rx_status->adapter[i].current_signal_dbm, i, td->rx_status->adapter[i].received_packet_cnt, td->rx_status->adapter[i].wrong_crc_cnt, td->rx_status->adapter[i].type, getWidth(6), getHeight(86 - i * 3.2), scale_factor*2.2);
	}
	#endif
    }
#endif

#ifdef OSD_RSSI
    if(td->rx_status_osd != NULL) {
	int i;
	int ac = td->rx_status_osd->wifi_adapter_cnt;
	int best_dbm = -1000;
	// find out which card has best signal
	for(i=0; i<ac; ++i) {
	    if (best_dbm < td->rx_status_osd->adapter[i].current_signal_dbm) best_dbm = td->rx_status_osd->adapter[i].current_signal_dbm;
	}
	draw_total_signal(best_dbm, td->rx_status_osd->received_block_cnt, td->rx_status_osd->damaged_block_cnt, td->rx_status_osd->lost_packet_cnt, td->rx_status_osd->received_packet_cnt, getWidth(80), getHeight(92), scale_factor*2.5, 255);
	#ifdef OSD_RSSI_DETAILED
	for(i=0; i<ac; ++i) {
//	    draw_card_signal(td->rx_status_osd->adapter[i].current_signal_dbm, i, td->rx_status_osd->adapter[i].received_packet_cnt, td->rx_status_osd->adapter[i].wrong_crc_cnt, getWidth(80), getHeight(82 - i * 2), scale_factor*1.7);
	    draw_card_signal(td->rx_status_osd->adapter[i].current_signal_dbm, i, td->rx_status_osd->adapter[i].received_packet_cnt, td->rx_status_osd->adapter[i].wrong_crc_cnt, td->rx_status->adapter[i].type, getWidth(80), getHeight(82 - i * 5), scale_factor*1.7);
	}
	#endif
    }
#endif

#ifdef RC_RSSI
    if(td->rx_status_rc != NULL) {
	draw_rc_signal(td->rx_status_rc->adapter[0].current_signal_dbm, td->rx_status_rc->lost_packet_cnt, getWidth(80), getHeight(92), scale_factor*2.5, 255);
    }
#endif

}



//taken from tinygps: https://github.com/mikalhart/TinyGPS/blob/master/TinyGPS.cpp#L296
float distance_between(float lat1, float long1, float lat2, float long2) {
  // returns distance in meters between two positions, both specified 
  // as signed decimal-degrees latitude and longitude. Uses great-circle 
  // distance computation for hypothetical sphere of radius 6372795 meters.
  // Because Earth is no exact sphere, rounding errors may be up to 0.5%.
  // Courtesy of Maarten Lamers
  float delta = (long1-long2)*0.017453292519;
  float sdlong = sin(delta);
  float cdlong = cos(delta);
  lat1 = (lat1)*0.017453292519;
  lat2 = (lat2)*0.017453292519;
  float slat1 = sin(lat1);
  float clat1 = cos(lat1);
  float slat2 = sin(lat2);
  float clat2 = cos(lat2);
  delta = (clat1 * slat2) - (slat1 * clat2 * cdlong); 
  delta = delta*delta; 
  delta += (clat2 * sdlong)*(clat2 * sdlong); 
  delta = sqrt(delta); 
  float denom = (slat1 * slat2) + (clat1 * clat2 * cdlong); 
  delta = atan2(delta, denom); 
  return delta * 6372795; 
}

//taken from tinygps: https://github.com/mikalhart/TinyGPS/blob/master/TinyGPS.cpp#L321
float course_to (float lat1, float long1, float lat2, float long2) 
{
  // returns course in degrees (North=0, West=270) from position 1 to position 2,
  // both specified as signed decimal-degrees latitude and longitude.
  // Because Earth is no exact sphere, calculated course may be off by a tiny fraction.
  // Courtesy of Maarten Lamers
  float dlon = (long2-long1)*0.017453292519;
  lat1 = (lat1)*0.017453292519;
  lat2 = (lat2)*0.017453292519;
  float a1 = sin(dlon) * cos(lat2);
  float a2 = sin(lat1) * cos(lat2) * cos(dlon);
  a2 = cos(lat1) * sin(lat2) - a2;
  a2 = atan2(a1, a2);
  if (a2 < 0.0)
  {
    a2 += M_PI*2;
  }
  return TO_DEG*(a2);
}


void rotatePoints(float *x, float *y, int angle, int points, int center_x, int center_y){
	double cosAngle = cos(-angle * 0.017453292519);
	double sinAngle = sin(-angle * 0.017453292519);

	int i = 0;
	int tmp_x = 0;
	int tmp_y = 0;
	while (i < points){
		tmp_x = center_x + (x[i]-center_x)*cosAngle-(y[i]-center_y)*sinAngle;
		tmp_y = center_y + (x[i]-center_x)*sinAngle + (y[i] - center_y)*cosAngle;
		x[i] = tmp_x;
		y[i] = tmp_y;
		i++;
	}
}


void draw_total_signal(int8_t signal, int goodblocks, int badblocks, int packets_lost, int packets_received, int data_received, int valid_msgs, int pos_x, int pos_y, float scale, int color){
    Fill(255,color,color,OPACITY);
    #if defined(DRAW_OUTLINE)
    Stroke(0,0,0,1);
    StrokeWidth(1);
    #endif

    sprintf(buffer, "%ddBm", signal);
    Text(pos_x, pos_y, buffer, SansTypeface, scale * 1.5);

    sprintf(buffer, "%d/%d/%d (%d/%d)", badblocks, packets_lost, packets_received, data_received, valid_msgs);
    Text(pos_x, getHeight(89), buffer, SansTypeface, scale_factor * 2.5);

//    sprintf(buffer, "T: %d/%d", data_received, valid_msgs);
//    Text(pos_x, getHeight(86), buffer, SansTypeface, scale_factor * 2.5);
}


void draw_rc_signal(int8_t signal, int packets_lost, int pos_x, int pos_y, float scale, int color){
    Fill(255,color,color,OPACITY);
    #if defined(DRAW_OUTLINE)
    Stroke(0,0,0,1);
    StrokeWidth(1);
    #endif
    sprintf(buffer, "%ddBm", signal);
//    Text(pos_x, pos_y, buffer, SansTypeface, scale);
    Text(pos_x, pos_y, buffer, SansTypeface, scale * 1.5);
    sprintf(buffer, "Lost: %d", packets_lost);
//    Text(pos_x, getHeight(89), buffer, SansTypeface, scale_factor * 2.1);
    Text(pos_x, getHeight(89), buffer, SansTypeface, scale_factor * 2.5);
}



void draw_card_signal(int8_t signal, int card, int packets, int wrongcrcs, int type, int pos_x, int pos_y, float scale){
    Fill(0xff,0xff,0xff,OPACITY);
    #if defined(DRAW_OUTLINE)
    Stroke(0,0,0,1);
    StrokeWidth(1);
    #endif

    if (type == 0) { // Atheros
	sprintf(buffer, "Rx%d(A): %ddBm (%d)", card, signal, packets);
    } else {
	sprintf(buffer, "Rx%d(R): %ddBm (%d)", card, signal, packets);
    }
    Text(pos_x, pos_y, buffer, SansTypeface, scale);
}



void paintArrow(int heading, int pos_x, int pos_y){
	if (heading == 360) heading = 0;

#ifdef INVERT_HOME_ARROW
	heading = 360 - heading;
#endif
	
	//offset for arrow, so middle of the arrow is at given position
	pos_x -= 20;
	pos_y -= 20;
	
	float x[8] = {10+pos_x, 10+pos_x, 0+pos_x, 20+pos_x, 40+pos_x, 30+pos_x, 30+pos_x, 10+pos_x};
	float y[8] = {0+pos_y, 20+pos_y, 20+pos_y, 40+pos_y, 20+pos_y,20+pos_y,0+pos_y,0+pos_y};
	Fill(0xff,0xff,0xff,OPACITY);
	#if defined(DRAW_OUTLINE)
	Stroke(0,0,0,1);
	StrokeWidth(2);
	#endif
	rotatePoints(x, y, heading, 8, pos_x+20,pos_y+20);
	Polygon(x, y, 8);
	Polyline(x, y, 8);
}

void paintAHI(int hor_angle, int ver_angle){
	//if vertical angle is larger than 45° leave the ahi at highest pos
	if (ver_angle > 45)
		ver_angle = 45;
	else if (ver_angle < -45)
		ver_angle = -45;

	int offset_x = (width / 6 * cos(hor_angle * 0.017453292519));
	int offset_y = (width / 6 * sin(hor_angle * 0.017453292519));
	
	int mid_y = getHeight(50);
	int horizon_y = (mid_y - 100) / 45 * ver_angle + mid_y;

	Stroke(0,0,0,1);
	StrokeWidth(5);
	//outer black border
	int mid_x = getWidth(50);
	Line(mid_x - offset_x,horizon_y - offset_y, mid_x + offset_x, horizon_y + offset_y);
	Stroke(0xff,0xff,0xff,1);
	StrokeWidth(2);
	//inner white line
	Line(mid_x - offset_x,horizon_y - offset_y, mid_x + offset_x, horizon_y + offset_y);
}

//new stuff from fritz walter https://www.youtube.com/watch?v=EQ01b3aJ-rk
void draw_bat_remaining(int remaining, int pos_x, int pos_y, float scale){
	//prevent black empty indicator to draw left to battery
	if (remaining < 0) remaining = 0;
	else if (remaining > 100) remaining = 100;

	int s_width = 20 * scale;
	int s_height = 10 * scale;
	int corner = 3 * scale;
	int plus_w = 2 * scale;
	int plus_h = 5 * scale;
	int stroke = 1 * scale;

	if (remaining <= CELL_WARNING_PCT1 && remaining > CELL_WARNING_PCT2){
		Fill(255,165, 0,OPACITY);
	}else if (remaining <= CELL_WARNING_PCT2){
		Fill(255,0, 0,OPACITY);
	}else{
		Fill(255, 255, 255,OPACITY);
	}
	
	#if defined(DRAW_OUTLINE)
	Stroke(0,0,0,1);
	StrokeWidth(1);
	#endif
	Roundrect(pos_x, pos_y , s_width, s_height, corner, corner);
	Rect(pos_x + s_width, pos_y + plus_h/2, plus_w, plus_h);

	Fill(0, 0, 0,OPACITY);
	Rect(pos_x + stroke + remaining / 100.0f * s_width, pos_y  + stroke, s_width - 2 * stroke - remaining / 100.0f * s_width, s_height - 2 * stroke);
}


void draw_compass(int heading, int pos_x, int pos_y, bool ladder_enabled, float scale){
	int width_number = 50 * scale;
	int height_number = 20 * scale;
	int space = 50 * scale;
	
//	fprintf(stdout,"draw compass\n");

	if (!ladder_enabled){
		pos_y += 20*scale;
	}
	//StrokeWidth(4);
	//Stroke(0,0,0,1); // black border
	//Fill(255,255,255,0);
	//Rect(pos_x - width_number / 2, pos_y - height_number/2 , width_number, height_number);

	StrokeWidth(2);
	Stroke(0xff,0xff,0xff,OPACITY);
	Fill(255,255,255,0);
	Rect(pos_x - width_number / 2, pos_y - height_number/2 , width_number, height_number);

	//Fill(0,0,0,1);
	Fill(0xff,0xff,0xff,OPACITY);
	#if defined(DRAW_OUTLINE)
	Stroke(0,0,0,1);
	StrokeWidth(1);
	#endif
	sprintf(buffer, "%3d\xb0", heading);
	TextMid(pos_x, pos_y - height_number / 4*1.5, buffer, SansTypeface, height_number / 2*1.5);

	if (ladder_enabled){
	int width_triangle = 20 * scale;
	
	int p0x = pos_x;
	int p0y = pos_y + space;
	int p1x = pos_x - width_triangle / 2;
	int p1y = pos_y + space + width_triangle / 2;
	int p2x = pos_x + width_triangle / 2;
	int p2y = p1y;

	VGfloat x2[] = { p0x, p1x, p2x, p0x };
	VGfloat y2[] = { p0y, p1y, p2y, p0y };
	Stroke(0, 0, 0, 1);
	Polyline(x2, y2, 4);
	Fill(255,255,255,0.5f);
	Polygon(x2, y2, 4);
	}
	

	if (ladder_enabled){
	int width_comp = 150 * scale;
	int height_comp = 20 * scale;
	float ratio = width_comp / 180.0f;

	int i = heading - 90;
	char* c;
	bool draw = false;
	//find all values from heading - 90 to heading + 90 that are % 15 == 0
	while (i <= heading + 90) {
		int x = pos_x + (i - heading) * ratio;
		if (i % 30 == 0) {
			Stroke(0,0,0,1);
			Fill(255, 255, 255, OPACITY);
			StrokeWidth(1);
			//outer black border
			Rect(x-1.5, pos_y +height_comp/3 +space /4, 3, height_comp/3 );
		}else if (i % 15 == 0) {
			Stroke(0,0,0,1);
			Fill(255,255,255,OPACITY);
			StrokeWidth(1);
			//outer black border
			Rect(x-1.5, pos_y + height_comp / 3 + space / 4, 3, height_comp/5);
		}else{
			i++;
			continue;
		}

		int j = i;
		if (j < 0)
			j += 360;
		if (j >= 360)
			j -= 360;

		switch (j) {
		case 0: {
			draw = true;
			c = "N";
			break;
		}
		case 90: {
			draw = true;
			c = "0";
			break;
		}
		case 180: {
			draw = true;
			c = "S";
			break;
		}
		case 270: {
			draw = true;
			c = "W";
			break;
		}
		}
		if (draw == true) {
			Stroke(0, 0, 0, 1);
			StrokeWidth(0.4);
			Fill(255,255,255,OPACITY);
			TextMid(x, pos_y + height_comp / 3 + space / 3 * 1.6f, c, SansTypeface, scale_factor * scale*1.5f);
			draw = false;
		}
		i++;
	}
	}
}

void draw_bat_status(float voltage, float current, int pos_x, int pos_y, float scale){
	Fill(0xff,0xff,0xff,OPACITY);
	#if defined(DRAW_OUTLINE)
	Stroke(0,0,0,1);
	StrokeWidth(1);
	#endif
	sprintf(buffer, "%.2fV", voltage);
	TextEnd(pos_x, pos_y, buffer, SansTypeface, scale);
#ifdef DRAW_CURRENT
	sprintf(buffer, "%.2fA", current);
	TextEnd(pos_x, pos_y + 40, buffer, SansTypeface, scale);
#endif
}

void draw_batt_mah(float voltage, float current, float pos_x, float pos_y, float scale){
    float text_scale = getWidth(2) * scale;

    VGfloat height_text = TextHeight(myfont, text_scale)+getHeight(0.3)*scale;
    if(first){ //little trick needed to be done to take the initial time
        lastT = current_ts();
        first = false;
    }
    timeDiff = current_ts() - lastT; //get the time difference between last time and new time
    mAhDrawRaw = current * timeDiff / 3600; //calculate the mAh
    mAhDrawn += mAhDrawRaw; //add the calculated mAh the total used

    lastT = current_ts(); //set lastT back to current time
    sprintf(buffer, "%.f", mAhDrawn);
    TextEnd(getWidth(pos_x), getHeight(pos_y), buffer, myfont, text_scale);
    Text(getWidth(pos_x), getHeight(pos_y), " mAh", myfont, text_scale*0.6);
}


void draw_position(float lat, float lon, bool fix, int sats, int fixtype, int pos_x, int pos_y, float scale){
	Fill(0xff,0xff,0xff,OPACITY);
	#if defined(DRAW_OUTLINE)
	Stroke(0,0,0,1);
	StrokeWidth(1);
	#endif

	sprintf(buffer, "Lon: %.6f", lon);
	Text(pos_x, pos_y, buffer, SansTypeface, scale);
	sprintf(buffer, "Lat: %.6f", lat);
	Text(pos_x, pos_y + 40, buffer, SansTypeface, scale);
	if (!fix){
		sprintf(buffer, "No valid fix!");
		Text(pos_x, pos_y + 80, buffer, SansTypeface, scale*.75f);
	}else /* if (sats > 0)*/{
		#if defined(LTM) || defined(MAVLINK)
		sprintf(buffer, "%d sats", sats);
		Text(pos_x, pos_y + 80, buffer, SansTypeface, scale*.75f); 
		#endif
	}
	
}

void draw_home_distance(int distance, int pos_x, int pos_y, float scale){
	Fill(0xff,0xff,0xff,OPACITY);
	#if defined(DRAW_OUTLINE)
	Stroke(0,0,0,1);
	StrokeWidth(1);
	#endif
#ifdef IMPERIAL
	sprintf(buffer, "%05dft", (int)(distance*TO_FEET));
#else
	sprintf(buffer, "%05dm", distance);
#endif
	TextMid(pos_x, pos_y, buffer, SansTypeface, scale);
}

//autopilot mode, mavlink specific, could be used if mode is in telemetry data of other protocols as well
void draw_mode(char *mode, int pos_x, int pos_y, float scale){
	TextEnd(pos_x, pos_y, mode, SansTypeface, scale_factor * scale);
}

void draw_home_indicator(int home_angle, int pos_x, int pos_y, float scale){
	//TODO use circle from openvg instead of this
	//center circle
	int radius = 3 * scale;
	StrokeWidth(2);
	Stroke(255, 255, 255, 1);
	float i;
	VGfloat x[100];
	VGfloat y[100];
	int z = 0;

	for (i = 0; i < 6.28f; i += 0.1f) {
		x[z] = sin(i) * radius + pos_x;
		y[z] = cos(i) * radius + pos_y;
		z++;
	}
	x[z] = x[0];
	y[z] = y[0];
	z++;
	Polyline(x, y, z);

	//home direction
	//todo home indication is wrong
	float length = 15 * scale;
	float arg = home_angle;
	VGfloat x1 = pos_x + sin(arg) * radius;
	VGfloat y1 = pos_y + cos(arg) * radius;
	VGfloat x2 = pos_x + sin(arg) * (radius + length);
	VGfloat y2 = pos_y + cos(arg) * (radius + length);
	Line(x1, y1, x2, y2);
}

void draw_altitude(int alt, int pos_x, int pos_y, bool ladder_enabled, float scale){
	// altitude label
	int space = 15 * scale;
	int s_width = 45 * scale;
	int s_height = 180 * scale;
	int label_height = 20 * scale;
	int pos_x_r = pos_x + s_width / 2 + width / 4 + space;
	int pos_y_r = pos_y;

	VGfloat x[6];
	VGfloat y[6];

	x[0] = pos_x_r;
	y[0] = pos_y_r;
	x[1] = pos_x_r + s_width / 5;
	y[1] = pos_y_r + label_height / 2;
	x[2] = x[1] + s_width;
	y[2] = y[1];
	x[3] = x[2];
	y[3] = y[2] - label_height;
	x[4] = x[1];
	y[4] = y[3];
	x[5] = x[0];
	y[5] = y[0];
	StrokeWidth(5);
	Stroke(0, 0, 0, 1);
	Polyline(x, y, 6);

	Stroke(0xff, 0xff, 0xff, 1);
	StrokeWidth(2);
	Polyline(x, y, 6);


	Fill(0xff,0xff,0xff,OPACITY);
	#if defined(DRAW_OUTLINE)
	Stroke(0,0,0,1);
	StrokeWidth(1);
	#endif
	sprintf(buffer, "%d", alt);
	TextMid(pos_x_r + s_width / 2 + s_width / 5, pos_y - scale_factor * scale*1.5 / 2, buffer, SansTypeface, scale_factor * scale*1.5);

	scale = 1.5;

	if (ladder_enabled){
		int width_ladder = 150 * scale;
		StrokeWidth(2);
		int width_speed = 50 * scale;
		int height_speed = 300 * scale;
		int length_bar = 5 * scale;

		int k;
		int space_text = 5 * scale;
		int distance = 100 * scale;

		//ladder altitude
		int range_altitude = 200;
		float ratio_altitude = height_speed / range_altitude;
		for (k = (int) (alt - range_altitude / 2); k <= alt + range_altitude / 2; k++) {
			int y = pos_y + (k - alt) * ratio_altitude;
			sprintf(buffer, "%d", k);
			if (k % 100 == 0) {
				int px = pos_x + width_ladder / 2 + distance + width_speed;
				int px2 = px + 2 * length_bar;

                                Stroke(0,0,0,1);
                                Fill(0xff,0xff,0xff,OPACITY);
                                StrokeWidth(1);
                                //outer black border
                                //Line(px, y, px2, y);
                                Rect(px-length_bar, y-1.5, 2*length_bar, 3);

				Fill(0xff,0xff,0xff,OPACITY);
				Stroke(0,0,0,1);
				StrokeWidth(0.4);
				Text(pos_x + width_ladder / 2 + distance + width_speed + space_text + 2 * length_bar, y - width / 240 * scale, buffer, SansTypeface, width / 300 * scale*2.5);
			}else if (k % 10 == 0) {
                                int px = pos_x + width_ladder / 2 + distance + width_speed;
                                int px2 = px + length_bar;
		  
		                Stroke(0,0,0,1);
                                Fill(0xff,0xff,0xff,OPACITY);
                                StrokeWidth(1);
                                //outer black border
                                //Line(px, y, px2, y);
                                Rect(px-length_bar, y-1.5, length_bar, 3);

                        }

		}
	}
}

void draw_speed(int speed, int pos_x, int pos_y, bool ladder_enabled, float scale){
	// velocity label
	StrokeWidth(2);
	Stroke(0, 0, 0, 1);
	int space = 15 * scale;
	int s_width = 45 * scale;
	int s_height = 180 * scale;
	int label_height = 20 * scale;
	int pos_x_r = pos_x + s_width / 2 + width / 4 + space;
	int pos_y_r = pos_y;

	VGfloat x[6];
	VGfloat y[6];

	int pos_x_l = pos_x - s_width / 2 - width / 4 - space;
	int pos_y_l = pos_y;
	x[0] = pos_x_l;
	y[0] = pos_y_l;
	x[1] = pos_x_l - s_width / 5;
	y[1] = pos_y_l + label_height / 2;
	x[2] = x[1] - s_width;
	y[2] = y[1];
	x[3] = x[2];
	y[3] = y[2] - label_height;
	x[4] = x[1];
	y[4] = y[3];
	x[5] = x[0];
	y[5] = y[0];
	StrokeWidth(5);
	Stroke(0, 0, 0, 1);
	Polyline(x, y, 6);

	Stroke(0xff, 0xff, 0xff, 1);
	StrokeWidth(2);
	Polyline(x, y, 6);
	
	Fill(0xff,0xff,0xff,OPACITY);
	#if defined(DRAW_OUTLINE)
	Stroke(0,0,0,1);
	StrokeWidth(1);
	#endif
	sprintf(buffer, "%d", speed);
	TextMid(pos_x_l - s_width / 2 - s_width / 5, pos_y - scale_factor * scale*1.5 / 2, buffer, SansTypeface, scale_factor * scale*1.5);
	
	scale = 1.5;
	if (ladder_enabled){
		//ladder speed
		int width_ladder = 150 * scale;

		StrokeWidth(2);
		int width_speed = 50 * scale;
		int height_speed = 300 * scale;
		int length_bar = 5 * scale;
		int range_speed = 40;

		int k;
		int space_text = 5 * scale;
		int distance = 100 * scale;
		float ratio_speed = height_speed / range_speed;
		for (k = (int) (speed - range_speed / 2); k <= speed + range_speed / 2; k++) {
			int y = pos_y + (k - speed) * ratio_speed;
			sprintf(buffer, "%d", k);
			if (k % 5 == 0) {
				int px = pos_x - width_ladder / 2 - distance - width_speed;
				int px2 = px - 2 * length_bar;
				Stroke(0,0,0,1);
				Fill(0xff,0xff,0xff,OPACITY);
				StrokeWidth(1);
				//outer black border
				//Line(px, y, px2, y);
				Rect(px-length_bar, y-1.5, 2*length_bar, 3);
				if (k >= 0){
					Fill(0xff,0xff,0xff,OPACITY);
					Stroke(0,0,0,1);
					StrokeWidth(0.4);			
					TextEnd(pos_x - width_ladder / 2 - distance - width_speed - space_text - 2 * length_bar, y - width / 240 * scale, buffer, SansTypeface, width / 300 * scale*2.5);
				}
			}else if (k % 1 == 0) {
                                int px = pos_x - width_ladder / 2 - distance - width_speed;
                                int px2 = px - length_bar;

                                Stroke(0,0,0,1);
                                Fill(0xff,0xff,0xff,OPACITY);
                                StrokeWidth(1);

                                //outer black border
                                //Line(px, y, px2, y);
                                Rect(px, y-1.5, length_bar, 3);
                                //Stroke(0xff,0xff,0xff,1);
                                //StrokeWidth(2);
                                //inner white line
                                //Line(px, y, px2, y);
                        }

		}
	}
}

void draw_horizon(float roll, float pitch, int pos_x, int pos_y, float scale){
	StrokeWidth(2);
	Stroke(255, 255, 255, 1);
	int height_ladder = 300 * scale;
	int width_ladder = 150 * scale;
	int range = 20;
	int space_text = 5 * scale;
	int pike = 4 * scale;
	float ratio = height_ladder / range;
	//int k;
	Translate(pos_x, pos_y);
	Rotate(roll);
	Translate(-pos_x, -pos_y);

	int y_start = height_ladder;

	int k = pitch - range/2;
	int max = pitch + range/2;
	while (k <= max){
		int y = pos_y + (k - pitch) * ratio;
		sprintf(buffer, "%d", k);
		if (k % 5 == 0 && k!= 0) {
			Fill(0xff,0xff,0xff,OPACITY);
			#if defined(DRAW_OUTLINE)
			Stroke(0,0,0,1);
			StrokeWidth(0.4);
			#endif
			TextEnd(pos_x - width_ladder / 2 - space_text, y - width / 300 * scale, buffer, SansTypeface, width / 300 * scale*2);
			Text(pos_x + width_ladder / 2 + space_text, y - width / 300 * scale, buffer, SansTypeface, width / 300 * scale*2);
		}
		if ((k > 0) && (k % 5 == 0)) {
			int px = pos_x - width_ladder / 2;
			int px2 = pos_x + width_ladder / 2;
			#if defined(DRAW_OUTLINE)
			Stroke(0,0,0,1);
			StrokeWidth(1);
			#endif
			Fill(255, 255, 255, OPACITY);
			Rect(px, y-1.5, width_ladder/3, 3);
			Rect(px+width_ladder*2/3, y-1.5, width_ladder/3, 3);
		} else if ((k < 0) && (k % 5 == 0)) {
			int px = pos_x - width_ladder / 2 +width_ladder / 3;
			int px3 = pos_x - width_ladder / 2 + 0.2055f * width_ladder;
			int px5 = pos_x - width_ladder / 2 + 0.077f * width_ladder;
			
			Fill(0xff,0xff,0xff,OPACITY);
		    	#if defined(DRAW_OUTLINE)
			Stroke(0,0,0,1);
			StrokeWidth(1);
			#endif
			//outer black border
			Rect(px, y-1.5, width_ladder/12, 3);
			Rect(px3, y-1.5, width_ladder/12, 3);
			Rect(px5, y-1.5, width_ladder/12, 3);

			px = pos_x + width_ladder / 2 -  width_ladder / 3;
			px3 = pos_x + width_ladder / 2 - 0.205f * width_ladder;
			px5 = pos_x + width_ladder / 2 - 0.077f * width_ladder;

			//outer black border
			Rect(px, y-1.5, width_ladder/12, 3);
			Rect(px3, y-1.5, width_ladder/12, 3);
			Rect(px5, y-1.5, width_ladder/12, 3);
		} else if (k == 0) {
			Fill(0xff,0xff,0xff,OPACITY);
		    	#if defined(DRAW_OUTLINE)
			Stroke(0,0,0,1);
			StrokeWidth(0.4);	
			#endif
			TextEnd(pos_x - width_ladder / 1.25f - space_text, y - width / 300 * scale, buffer, SansTypeface, width / 300 * scale*2);
			Text(pos_x + width_ladder / 1.25f + space_text, y - width / 300 * scale, buffer, SansTypeface, width / 300 * scale*2);
		    	#if defined(DRAW_OUTLINE)
			Stroke(0,0,0,1);
			StrokeWidth(1);
			#endif
			Fill(0xff,0xff,0xff,OPACITY);
			//outer black border
			Rect(pos_x - width_ladder / 1.25f, y-1.5, 2*(width_ladder /1.25f), 3);
		}
		k++;
	}
}
