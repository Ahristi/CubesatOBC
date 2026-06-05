#ifndef __PACKET_H__
#define __PACKET_H__
#include <stdint.h>

//-------------DEFINES--------------
#define MAX_PACKET_SIZE 192


//-------------Typedefs and Enums--------------
typedef struct{
    uint8_t id;
    uint16_t packet_idx;
    uint8_t payload[MAX_PACKET_SIZE];
    uint8_t length;
}Packet_t;

#endif