#include "osdconfig.h"

#ifdef MAVLINK
#include "telemetry.h"
#include "mavlink/mavlink.h"

void mavlink_read(telemetry_data_t *td, uint8_t *buf, int buflen);
#endif
