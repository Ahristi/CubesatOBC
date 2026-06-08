#ifndef __DEBUG_H__
#define __DEBUG_H__
#include <stdbool.h>
#include "adcs.h"
//-------------Defines-------------
#define DEBUG_PERIOD_MS 1000


//-------------Typedefs and Enums-------------
typedef enum{
    DEBUG_TOGGLE                = 0x30,
    DEBUG_TEST_X_RW             = 0x31,
    DEBUG_TEST_Y_RW             = 0x32,
    DEBUG_TEST_Z_RW             = 0x33,
    DEBUG_TEST_X_MAG            = 0x34,
    DEBUG_TEST_Y_MAG            = 0x35,
    DEBUG_TEST_Z_MAG            = 0x37,
    DEBUG_TEST_PAYLOAD          = 0x38,
    DEBUG_TEST_CAMERA           = 0x39,
    DEBUG_TEST_ADCS_EFUSE       = 0x40,
    DEBUG_TEST_PAYLOAD_EFUSE    = 0x41,
    DEBUG_TEST_EXIT             = 0x2,
}Debug_ID_t;



typedef struct{
    bool request_debug_mode;
    bool debug_enable;
    float xrw;
    float yrw;
    float zrw;
    float xmag;
    float ymag;
    float zmag;
    bool adcs_efuse_test;
    bool payload_efuse_test;
    bool payload_test;
    bool camera_test;
    ADCS_adcsDebugCommand_t adcs_cmd;
}DEBUG_Handler_t;
//-------------Function Prototypes-------------
void DEBUG_Init(void);
void DEBUG_Task(void);
void DEBUG_update(uint8_t ID);

extern DEBUG_Handler_t hdebug;
#endif