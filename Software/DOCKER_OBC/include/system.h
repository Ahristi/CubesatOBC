#ifndef SYSTEM_H
#define SYSTEM_H
#include <Arduino.h>
#include "can.h"
#include "eps.h"
#include "adcs.h"
#include "comms.h"
#include "logging.h"
#include "scheduler.h"
#include "math.h"
//-------------Defines-------------
#define SYSTEM_LED0_PIN     43
#define SYSTEM_LED1_PIN     42
#define LONG_PAUSE_TICKS    10
#define WATHDOG_TICKS       100

#define OBC_WATCHDOG_ID     0x69
#define OBC_WATCHDOG_CODE   0x64


#define PAYLOAD_TIMEOUT_TICKS 120000 //Around 10 minutes

//-------------Typedefs and Enums-------------
typedef enum{
    LONG_PAUSE,
    FIRST_PULSE,
    SHORT_PAUSE,
    SECOND_PULSE
}SYSTEM_HeartbeatState_t;

typedef enum{
    INIT,
    DETUMBLE,
    IDLE,
    EXPERIMENT,
    LINK,
    DEBUG
}SYSTEM_State_t;



typedef struct{
    SYSTEM_State_t state;
    SYSTEM_State_t previous_state;
    bool state_entry;
    SYSTEM_HeartbeatState_t stat_led_state;
    uint8_t heartbeat_tick_count;
    uint8_t watchdog_tick_count;
}SYSTEM_Handler_t;

typedef struct{
    bool debug_request;
    bool xrw_test;
    bool yrw_test;
    bool zrw_test;
    bool xmag_test;
    bool ymag_test;
    bool zmag_test;
    bool adcs_efuse_test;
    bool payload_efuse_test;
    bool payload_test;
    bool camera_test;
}SYSTEM_DEBUG_Handler_t;


extern SYSTEM_DEBUG_Handler_t hdebug;


//-------------Function Prototypes-------------
void SYSTEM_Init(void);
void SYSTEM_task(void);
void SYSTEM_heartbeat(void);
void SYSTEM_watchdog(void);
void SYSTEM_stateMachine(void);
void SYSTEM_setState(SYSTEM_State_t new_state);
bool SYSTEM_isEnteringState(void);
void SYSTEM_debugUpdate(uint8_t ID);
void SYSTEM_debugPrint(void);
//-------------Variables-------------




#endif