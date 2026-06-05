#ifndef COMMS_H
#define COMMS_H
#include <stdint.h>
#include <string.h>
#include "uart.h"
#include "file.h"
#include "comms.h"
#include "logging.h"
#include "payload.h"
#include "packet.h"


//-------------DEFINES--------------
#define COMMS_BAUDRATE 3000000
#define BEACON_TICK_OC 200

#define CUBESAT_IDENTIFIER "DOCKER"
#define CUBESAT_IDENTIFIER_BYTES 6
#define BEACON_TIME_STRING_BYTES 32



#define EXPERIMENT_COMMAND_ID  0x13
#define RESULT_REQUEST_ID      0x14

#define BEACON_MSG_ID          0x65
#define WOD_INFO_ID            0x66
#define WOD_REQUEST_ID         0x67
#define COMMS_ACK_ID           0x68
#define CHUNK_ID               0x69
#define END_TRANSFER_ID        0x70
#define UPLINK_FILE_INFO_ID    0x75


#define WOD_INFO_BYTES  9
#define FILE_INFO_BYTES 9
#define MAX_ACK_RETRIES 300
#define UPLINK_TIMEOUT  300


//-------------Typedefs and Enums--------------
typedef struct __attribute__((packed)) {
    char utc_time[BEACON_TIME_STRING_BYTES];
    uint8_t identifier[CUBESAT_IDENTIFIER_BYTES];

    //EPS Telemetry
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
    uint8_t  battery_temp;
    uint16_t mcu_temp;
    uint8_t  charger_die_temp;
    uint16_t mppt1_voltage;
    uint16_t mppt2_voltage;
    uint16_t mppt1_current;
    uint16_t mppt2_current;
    uint8_t  eFuse_states;
    uint8_t  eFuse_faults;

    //ADCS Telemetry
    float    roll;
    float    pitch;
    float    yaw;
    float    omega_x;
    float    omega_y;
    float    omega_z;
    float    x_rw_speed;
    float    y_rw_speed;
    float    z_rw_speed;
    float    x_mag_current;
    float    y_mag_current;
    float    z_mag_current;
    
    float    x_mag_field_sense;
    float    y_mag_field_sense;
    float    z_mag_field_sense;
    
    float    x_mag_field_filt;
    float    y_mag_field_filt;
    float    z_mag_field_filt;

    float    detumble_scale;
    
    uint16_t EPS_Faults;
    uint16_t OBC_Faults;
    uint16_t ADCS_Faults;
    uint16_t Payload_Faults;
    uint16_t Comms_Faults;
}COMMS_BeaconData_t;





typedef enum {
    COMMS_IDLE,
    COMMS_WOD_DOWNLINK, 
    COMMS_RESULTS_DOWNLINK,
    COMMS_EXPERIMENT_UPLINK,
    COMMS_COMMAND_UPLINK,
}COMMS_state_t;


typedef enum {
    DOWNLINK_IDLE,
    DOWNLINK_SEND_INFO,
    DOWNLINK_SEND_CHUNK,
    DOWNLINK_WAIT_ACK,
    DOWNLINK_COMPLETE,
    DOWNLINK_ERROR
}COMMS_downlinkState_t;

typedef struct {
    COMMS_downlinkState_t state;
    COMMS_downlinkState_t prev_state;
    uint16_t ack_retries;
}COMMS_downlinkHandler_t;

typedef enum {
    UPLINK_IDLE,
    UPLINK_RECEIVE_INFO,
    UPLINK_RECEIVE_PACKET,
    UPLINK_SEND_ACK,
    UPLINK_COMPLETE,
    UPLINK_ERROR
}COMMS_uplinkState_t;


typedef struct {
    COMMS_uplinkState_t state;
    COMMS_uplinkState_t prev_state;
    uint16_t timeout_ctr;
    uint32_t file_chunks;
}COMMS_uplinkHandler_t;


typedef struct{
    uint32_t beacon_tick;
    COMMS_state_t state;
}COMMS_Handler_t;

extern COMMS_Handler_t hcomms;


//-------------Function Prototypes--------------
void COMMS_Init(void);
void COMMS_task(void);
void COMMS_sendBeacon(void);
void COMMS_packBeacon(COMMS_BeaconData_t* data);
void COMMS_downLinkHandler(void);
void COMMS_startDownlink(const char *filename, uint16_t file_id);
void COMMS_wodDownlinkHandler(void);
void COMMS_sendFileInfo(uint8_t fileID, uint32_t chunk_size, uint32_t num_chunks);
bool COMMS_getLink(COMMS_Handler_t* hcomms);
bool COMMS_sendWOD(void);
bool COMMS_getAck(void);
bool COMMS_sendAck(void);
bool COMMS_sendEndTransfer(void);
bool COMMS_getEndTransfer(void);
void COMMS_downlink(FILE_Handler_t* hfile, COMMS_downlinkHandler_t* hdownlink);
void COMMS_uplink(FILE_Handler_t* hfile, COMMS_uplinkHandler_t* huplink);
bool COMMS_sendPacket(uint8_t id, const uint8_t *payload, uint8_t length);
bool COMMS_receivePacket(Packet_t* packet);
bool COMMS_receiveFileInfo(FILE_Handler_t* hfile, COMMS_uplinkHandler_t* huplink);
#endif