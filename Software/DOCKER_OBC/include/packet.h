#ifndef __PACKET_H__
#define __PACKET_H__
#include <stdint.h>
#include "uart.h"
//-------------DEFINES--------------
#define MAX_PACKET_SIZE 192
#define PACKET_INDEX_BYTES 2

//-------------Typedefs and Enums--------------
typedef struct{
    uint8_t id;
    uint16_t packet_idx;
    uint8_t payload[MAX_PACKET_SIZE];
    uint8_t length;
}Packet_t;

//-------------Function Prototypes--------------
bool COMMS_sendPacket(Packet_t* packet, HardwareSerialIMXRT* port);

#endif