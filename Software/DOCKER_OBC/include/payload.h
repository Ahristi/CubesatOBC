#ifndef PAYLOAD_H
#define PAYLOAD_H
#include "logging.h"
#include <stdbool.h>
#include "file.h"
//-------------Defines-------------

//File IDs
#define RESULT_FILE_ID           0x05
#define EXPERIMENT_META_ID       0x07



#define PAYLOAD_BAUD_RATE        3000000


//UART message IDS
#define PAYLOAD_START_ID         0x10

#define EXPERIMENT_CHUNK_ID      0x11
#define PAYLOAD_ACK_ID           0x68
#define PAYLOAD_END_TRANSFER_ID  0x70





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






//-------------Typedefs and Enums-------------
typedef enum{
    PAYLOAD_OFF,
    PAYLOAD_BOOT,
    PAYLOAD_SEND_EXPERIMENT,
    PAYLOAD_WAIT_ACK,
    PAYLOAD_SEND_END_FILE,
    PAYLOAD_RUNNING,
    PAYLOAD_GET_RESULTS,
    PAYLOAD_SEND_ACK,
    PAYLOAD_END
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
    uint16_t indx;
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

    bool experiment_ready;
    bool start_experiment;
    bool experiment_finished;
    FILE_Handler_t experiment_file;
    FILE_Handler_t results_file;
    HardwareSerialIMXRT* serial;
}PAYLOAD_Handler_t;

extern PAYLOAD_Handler_t hpayload;


//-------------Function Prototypes-------------
void PAYLOAD_Init();
void PAYLOAD_task();
bool PAYLOAD_getStartCMD(void);
bool PAYLOAD_sendChunk(uint8_t id, const uint8_t *payload, uint8_t length);
bool PAYLOAD_sendAck(void);
bool PAYLOAD_getAck(void);
bool PAYLOAD_sendEndTransfer(void);
#endif