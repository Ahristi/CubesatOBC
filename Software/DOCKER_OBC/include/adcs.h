#ifndef ADCS_H
#define ADCS_H
#include <stdint.h>
#include "uart.h"

//-------------Defines-------------
#define DETUMBLE_RATE_THRESHOLD 0.1 
#define DETUMBLE_SCALE_START 10.0f
#define ADCS_BAUDRATE 3000000


#define ADCS_SOF 0x64
#define ADCS_RX_BUFFER_LEN 64



//ADCS Telemetry
#define ADCS_PACKET_TELEMETRY 0x80
#define ADCS_PACKET_ACK       0xF0
#define ADCS_PACKET_ERROR     0xFF
#define ADCS_PACKET_TELEMETRY_BYTES 52
#define ADCS_PACKET_ACK_BYTES        1
#define ADCS_PACKET_ERROR_BYTES      1

#define ADCS_RX_CRC_BYTES 2

//ADCS COMMANDS
#define ADCS_ATTITUDE_UPDATE_ID  0x13
#define ADCS_ATTITUDE_UPDATE_BYTES 24

#define ADCS_ORBIT_UPDATE_ID    0x20
#define ADCS_ORBIT_UPDATE_BYTES   24
//-------------Typedef and Enums-------------



typedef struct {
    float roll;
    float pitch;
    float yaw;


    float omega_x;
    float omega_y;
    float omega_z;
}ADCS_attitudeCommand_t;

typedef struct {
    float t_gmt;
    float mean_anomaly;
    float eccentricity;
    float argp;
    float raan;
    float inclination;
}ADCS_orbitalParameters_t;


typedef enum{
    ADCS_RX_WAIT_SOF,
    ADCS_RX_GET_ID,
    ADCS_RX_READ_PAYLOAD,
    ADCS_RX_CHECK_CRC
}ADCS_rx_state_t;




typedef struct __attribute__((packed)) {
    float roll;
    float pitch;
    float yaw;

    float omega_x;
    float omega_y;
    float omega_z;

    float rw1;
    float rw2;
    float rw3;

    float it1;
    float it2;
    float it3;

    float x_mag_field_sense;
    float y_mag_field_sense;
    float z_mag_field_sense;
    
    float x_mag_field_filt;
    float y_mag_field_filt;
    float z_mag_field_filt;

    float detumble_scale;
    uint16_t faults;
    
}ADCS_TelemetryPacket_t;

typedef struct {
    float detumble_scale;

    //Commands
    ADCS_attitudeCommand_t attitude_command;
    ADCS_orbitalParameters_t orbital_parameters;

    //Telemetry
    ADCS_TelemetryPacket_t telemetry; 

    //OBC->ADCS Command Ready flags
    bool detumble_command_ready;
    bool pointing_command_ready;
    bool attitude_command_ready;
    bool orbital_parameters_ready;
}ADCS_Handler_t;



//-------------Variables-------------
extern ADCS_Handler_t hadcs;


//-------------Function Prototypes-------------
void ADCS_Init(void);
void ADCS_task(void);
void ADCS_getTelemetry(void);
void ADCS_telemetryHandle(void);
void ADCS_processPacket(uint8_t id,uint8_t* packet, uint8_t packet_len);
void ADCS_updateAttitude(void);
void ADCS_updateOrbitalParameters(void);
uint8_t ADCS_getRxPayloadLength(uint8_t);
void ADCS_debugPrint(void);

#endif