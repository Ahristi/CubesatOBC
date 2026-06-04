#ifndef ADCS_COMMAND_H
#define ADCS_COMMAND_H

#include <stdint.h>
#include "uart.h"

#define ADCS_COMMAND_ID 0xA3

typedef enum {
    RW_X_POS,
    RW_X_NEG,
    RW_Y_POS,
    RW_Y_NEG,
    RW_Z_POS,
    RW_Z_NEG,
    MT_X_POS,
    MT_X_NEG,
    MT_Y_POS,
    MT_Y_NEG,
    MY_Z_POS,
    MT_Z_NEG
} ADCS_Command_Key_t;

/*
    The purpose of ths code is to define commands from OBC
    To ADCS. It is to be used for comms to command ADCS.
*/

typedef struct __attribute__((packed)) {

    bool obey_commands;

    ADCS_Command_Key_t command;


} ADCS_Coms_Command_Handle_t;


extern ADCS_Coms_Command_Handle_t adcsCommandHandle;


#endif