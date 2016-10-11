#define JOY_DEV "/dev/input/js0"
#define NAME_LENGTH 128
#define KEEPALIVE 200 //in milliseconds, when there is more than KEEPALIVE ms elapsed we resend the packet even if it is the same
#define PACKET_LENGTH 26
#define MAX_RATE 2000 //max 1 packet every MAX_RATE microseconds

//reattribute those channels based on your needs
int CHANNEL_INDEX[8]={1, 4, 3, 0, 2, 5, 6, 7};