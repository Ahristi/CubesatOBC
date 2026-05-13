#ifndef COMMS_H
#define COMMS_H
#include <stdint.h>
#include "uart.h"
#include "adcs.h"
#include "eps.h"

//-------------DEFINES--------------
#define COMMS_BAUDRATE 3000000
#define BEACON_TICK_OC 200

#define CUBESAT_IDENTIFIER "DOCKER"
#define CUBESAT_IDENTIFIER_BYTES 6
//-------------Typedefs and Enums--------------

typedef struct{
    uint32_t beacon_tick;
}COMMS_Handler_t;


typedef struct{
    uint32_t utc_time;
    uint8_t identifier[CUBESAT_IDENTIFIER_BYTES];
    LOGGING_EPSTelemetry_t epsTelem;
    LOGGING_ADCSTelemetry_t adcsTelem;
}COMMS_BeaconData_t;



extern COMMS_Handler_t hcomms;


//-------------Function Prototypes--------------
void ADCS_Init(void);
void COMMS_task(void);
void COMMS_sendBeacon(void);


#endif