#ifndef PAYLOAD_H
#define PAYLOAD_H
#include "logging.h"
#include <stdbool.h>
#include "file.h"
//-------------Defines-------------
#define PAYLOAD_TASK_PERIOD_MS 5
#define PAYLOAD_START_TIMEOUT_SECONDS 360
#define PAYLOAD_START_TIMEOUT_COUNT (PAYLOAD_START_TIMEOUT_SECONDS*1000)/PAYLOAD_TASK_PERIOD_MS
#define PAYLOAD_ACK_TIMEOUT_MS 200
#define PAYLOAD_ACK_TIMEOUT_COUNT PAYLOAD_ACK_TIMEOUT_MS/PAYLOAD_TASK_PERIOD_MS
#define PAYLOAD_EXPERIMENT_TIMEOUT_SECONDS 360
#define PAYLOAD_EXPERIMENT_TIMEOUT_COUNT PAYLOAD_EXPERIMENT_TIMEOUT_SECONDS*1000/PAYLOAD_TASK_PERIOD_MS



//File IDs
#define RESULT_META_ID           0x05
#define EXPERIMENT_META_ID       0x07

#define PAYLOAD_BAUD_RATE        115200


//UART message IDS
#define PAYLOAD_ON_ID               0xB0
#define PAYLOAD_START_CMD_ID        0xB1
#define PAYLOAD_STOP_CMD_ID         0xB2
#define PAYLOAD_DEBUG_CMD_ID        0xB3
#define PAYLOAD_REQUEST_TRANSFER_ID 0xB4
#define PAYLOAD_FILE_INFO_ID        0x66 //Same ID used in comms

#define EXPERIMENT_CHUNK_ID         0x69
#define RESULTS_CHUNK_ID            0x69
#define PAYLOAD_ACK_ID              0x68 //Same ID used in comms
#define PAYLOAD_END_TRANSFER_ID     0x70


//UART message lengths
#define PAYLOAD_FILE_INFO_BYTES 9

#define PAYLOAD_UART_BUFFER_SIZE 1024

//These are technically wrong, but will give us enough records i'm pretty sure
#define EXPERIMENT_BUFFER_BYTES (1024UL * 1024UL * 1024UL) //1GiB
#define EXPERIMENT_RECORD_BYTES sizeof(PAYLOAD_ExperimentRecord_t)
#define EXPERIMENT_MAX_RECORDS  (EXPERIMENT_BUFFER_BYTES / EXPERIMENT_RECORD_BYTES)

#define RESULT_BUFFER_BYTES (1024UL * 1024UL * 1024UL) //1GiB
#define RESULT_MAX_RECORDS  (RESULT_BUFFER_BYTES / MAX_CHUNK_SIZE)


#define RESULT_META_FILE_NAME           "/result_metadata.bin"
#define RESULT_DATA_FILE_NAME           "/result.bin"

#define EXPERIMENT_META_FILE_NAME       "/experiment_metadata.bin"
#define EXPERIMENT_DATA_FILE_NAME       "/experiment.bin"

#define PAYLOAD_MAX_ACK_RETRIES 3

//-------------Typedefs and Enums-------------
typedef enum{
    PAYLOAD_OFF,
    PAYLOAD_BOOT,
    PAYLOAD_SEND_INFO,
    PAYLOAD_SEND_EXPERIMENT,
    PAYLOAD_START_EXPERIMENT,
    PAYLOAD_WAIT_ACK,
    PAYLOAD_SEND_END_FILE,
    PAYLOAD_RUNNING,
    PAYLOAD_GET_RESULTS,
    PAYLOAD_SEND_ACK,
    PAYLOAD_END,
    PAYLOAD_ERROR
}PAYLOAD_State_t;



typedef struct __attribute__((packed)) {
    float frame_rate;
    float threshold;
    float exposure;
    float satellite_attitude;
    uint16_t num_records;
    uint16_t record_size;
} PayloadCommandHeader_t;

typedef struct __attribute__((packed)){
    float pos_x;
    float pos_y;
    float pos_z;
    float roll;
    float pitch;
    float yaw;
}PAYLOAD_ExperimentRecord_t;



typedef struct{
    PAYLOAD_State_t prev_state;
    PAYLOAD_State_t state;

    uint8_t ack_retry_ctr;
    bool experiment_ready;
    bool start_experiment;
    bool experiment_finished;
    uint32_t num_result_chunks;
    uint32_t timeout_ctr;
    FILE_Handler_t experiment_file;
    FILE_Handler_t results_file;
    HardwareSerialIMXRT* serial;
}PAYLOAD_Handler_t;

extern PAYLOAD_Handler_t hpayload;


//-------------Function Prototypes-------------
void PAYLOAD_Init();
void PAYLOAD_task();
bool PAYLOAD_getOnMSG(void);
void PAYLOAD_start(void);
bool PAYLOAD_sendFileInfo(FILE_Handler_t* hfile, HardwareSerialIMXRT* port);
bool PAYLOAD_receiveFileInfo(FILE_Handler_t* hfile, HardwareSerialIMXRT* port);
bool PAYLOAD_receiveChunk(void);
bool PAYLOAD_sendAck(void);
bool PAYLOAD_getAck(void);
bool PAYLOAD_getEndTransfer(void);
bool PAYLOAD_sendEndTransfer(void);
void PAYLOAD_setState(PAYLOAD_State_t);
#endif