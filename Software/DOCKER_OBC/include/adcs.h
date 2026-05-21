#ifndef ADCS_H
#define ADCS_H
#include <stdint.h>
#include <Arduino.h>
#include "uart.h"

#define HW_TO_STATE_SERIAL &Serial2
#define STATE_TO_HW_SERIAL &Serial2

//-------------Defines-------------
#define DETUMBLE_RATE_THRESHOLD 0.1 
#define DETUMBLE_SCALE_START 10.0f
#define ADCS_BAUDRATE 115200


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



// ADCS STATE CONTROLLER COMMANDS AND HARDWARE FEEDBACK
#define ADCS_PACKET_REQUEST_ID    0x7A
#define ADCS_PACKET_REQUEST_BYTES    1


#define ADCS_PACKET_READY_ID      0x77
#define ADCS_PACKET_READY_BYTES    1

#define ADCS_PACKET_HW_DATA_ID    0x5D
#define ADCS_PACKET_HW_DATA_BYTES    1

#define ADCS_PACKET_CONTROL_ID    0x3F
#define ADCS_PACKET_CONTROL_BYTES    1

//-------------Typedef and Enums-------------



typedef struct {
    float roll;
    float pitch;
    float yaw;

    float roll_dot;
    float pitch_dot;
    float yaw_dot;
}ADCS_attitudeCommand_t;

typedef struct {
    float t_gmt;
    float mean_anomaly;
    float eccentricity;
    float argp;
    float raan;
    float inclination;
}ADCS_orbitalParameters_t;

typedef struct {
    bool request;
}ADCS_requestHardwareData_t;

typedef struct {
    bool ready;
}ADCS_hardwareReady_t;

typedef struct {
    float omega_rw_x;
    float omega_rw_y;
    float omega_rw_z;

    float I_mt_x;
    float I_mt_y;
    float I_mt_z;
} ADCS_hardwareData_t;

typedef struct {
    float tau_rw_x;
    float tau_rw_y;
    float tau_rw_z;
    
    float I_mt_x;
    float I_mt_y;
    float I_mt_z;
} ADCS_hardwareInstruction_t;


typedef enum{
    ADCS_RX_WAIT_SOF,
    ADCS_RX_GET_ID,
    ADCS_RX_READ_PAYLOAD,
    ADCS_RX_CHECK_CRC
}ADCS_rx_state_t;

typedef struct {
    float detumble_scale;

    //Commands
    ADCS_attitudeCommand_t attitude_command;
    ADCS_orbitalParameters_t orbital_parameters;

    //Attitude
    float roll;
    float pitch;
    float yaw;

    //Angular Velocity
    float roll_dot;
    float pitch_dot;
    float yaw_dot;

    //Reaction Wheel Speeds
    float rw1;
    float rw2;
    float rw3;

    //Magnetorquer speeds
    float it1;
    float it2;
    float it3;

    bool detumble_command_ready;
    bool pointing_command_ready;
    bool attitude_command_ready;
    bool orbital_parameters_ready;
}ADCS_Handler_t;


typedef struct{
    bool ready;
    ADCS_hardwareData_t hw_data;
    ADCS_hardwareInstruction_t ctrl;
} ADCS_HardwareHandler_t;

typedef struct{
    float R_G_from_B[3][3];

    float omega_B_in_B[3];

    float theta_dot_fws[3];

} ADCS_StateHandle_t;


//-------------Variables-------------
extern ADCS_Handler_t hadcs;
extern ADCS_HardwareHandler_t hwHandle;
extern ADCS_StateHandle_t stateHandle;
extern ADCS_hardwareInstruction_t controlHandle;


//-------------Function Prototypes-------------
void ADCS_Init(void);
void ADCS_task(void);
void ADCS_getTelemetry(void);
void ADCS_receivePacket(Stream *port);
void ADCS_processPacket(uint8_t id,uint8_t* packet, uint8_t packet_len);
void ADCS_updateAttitude(void);
void ADCS_updateOrbitalParameters(void);
uint8_t ADCS_getRxPayloadLength(uint8_t);
void ADCS_debugPrint(void);

// ADCS Hardware/state controller comms
void ADCS_getStateControllerUART(Stream* port);
void ADCS_sendHardwareRequest(Stream* port);
void ADCS_sendHardwareReady(Stream* port);
void ADCS_hardwareRequestResponse(Stream* port);
void ADCS_sendControlInstruction(Stream* port);

#endif