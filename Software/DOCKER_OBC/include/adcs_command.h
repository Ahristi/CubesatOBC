#ifndef ADCS_COMMAND_H
#define ADCS_COMMAND_H

#include <stdint.h>
#include "uart.h"

#define ADCS_DEBUG_ID 0xA3
/*
    The purpose of ths code is to define commands from OBC
    To ADCS. It is to be used for comms to command ADCS.
*/

typedef struct __attribute__((packed)) {
    bool debug_enable;  //Bool to command debug mode on the ADCS side
    float xrw;          //Commanded x reaction wheel speed in rpm
    float yrw;          //Commanded y reaction wheel speed in rpm
    float zrw;          //Commanded z reaction wheel speed in rpm 
    float xmag;         //Commanded x magnetorquer current in mA
    float ymag;         //Commanded y magnetorquer current in mA
    float zmag;         //Commanded z magnetorquer current in mA 
} ADCS_debugCommand_t;

extern ADCS_debugCommand_t adcsCommandHandle;


#endif