#ifndef PAYLOAD_H
#define PAYLOAD_H
#include "logging.h"
#include <stdbool.h>

//-------------Defines-------------
#define RESULT_META_ID           0x524553UL  // "RES"
#define EXPERIMENT_META_ID       0x504C44UL  // "PLD"

#define EXPERIMENT_BUFFER_BYTES (1024UL * 1024UL * 1024UL) //1GiB
#define EXPERIMENT_RECORD_BYTES sizeof(PAYLOAD_ExperimentRecord_t)
#define EXPERIMENT_MAX_RECORDS  (RESULT_BUFFER_BYTES / RESULT_RECORD_BYTES)






//-------------Typedefs and Enums-------------
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





//-------------Function Prototypes-------------
void PAYLOAD_Init();
void PAYLOAD_task();



#endif