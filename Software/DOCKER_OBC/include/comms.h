#ifndef COMMS_H
#define COMMS_H
#include <stdint.h>
#include <string.h>
#include "uart.h"
#include "logging.h"
#include "adcs.h"
#include "eps.h"

//-------------DEFINES--------------
#define COMMS_BAUDRATE 3000000
#define BEACON_TICK_OC 200

#define CUBESAT_IDENTIFIER "DOCKER"
#define CUBESAT_IDENTIFIER_BYTES 6
#define BEACON_TIME_STRING_BYTES 32

#define BEACON_MSG_ID   0x65
#define BEACON_MSG_DATA_BYTES  72




//-------------Typedefs and Enums--------------

typedef struct{
    uint32_t beacon_tick;
}COMMS_Handler_t;


typedef struct __attribute__((packed)) {
    char utc_time[BEACON_TIME_STRING_BYTES];
    uint8_t identifier[CUBESAT_IDENTIFIER_BYTES];
    uint16_t rail_3v3_voltage;
    uint16_t rail_3v3_current_ch1;
    uint16_t rail_3v3_current_ch2;
    uint16_t rail_5v_voltage;
    uint16_t rail_5v_current_ch1;
    uint16_t rail_5v_current_ch2;
    uint16_t rail_6v_voltage;
    uint16_t rail_6v_current_ch1;
    uint16_t rail_6v_current_ch2;
    uint16_t rail_12v_voltage;
    uint16_t rail_12v_current_ch1;
    uint16_t rail_12v_current_ch2;
    uint16_t battery_voltage;
    uint16_t sys_voltage;
    uint16_t battery_current;
    uint8_t battery_temp;
    uint16_t mcu_temp;
    uint8_t  charger_die_temp;
    uint16_t mppt1_voltage;
    uint16_t mppt2_voltage;
    uint16_t mppt1_current;
    uint16_t mppt2_current;
    uint8_t eFuse_states;
    uint8_t eFuse_faults;
    uint16_t roll;
    uint16_t pitch;
    uint16_t yaw;
    uint16_t x_rw_speed;
    uint16_t y_rw_speed;
    uint16_t z_rw_speed;
    uint16_t x_mag_current;
    uint16_t y_mag_current;
    uint16_t z_mag_current;
    uint16_t EPS_Faults;
    uint16_t OBC_Faults;
    uint16_t ADCS_Faults;
    uint16_t Payload_Faults;
    uint16_t Comms_Faults;
}COMMS_BeaconData_t;



extern COMMS_Handler_t hcomms;


//-------------Function Prototypes--------------
void COMMS_Init(void);
void COMMS_task(void);
void COMMS_sendBeacon(void);
void COMMS_packBeacon(COMMS_BeaconData_t* data);

#endif