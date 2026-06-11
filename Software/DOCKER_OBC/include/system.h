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
#include "debug.h"
//-------------Defines-------------
#define SYSTEM_LED0_PIN     43
#define SYSTEM_LED1_PIN     42
#define LONG_PAUSE_TICKS    10
#define WATHDOG_TICKS       100

#define OBC_WATCHDOG_ID     0x69
#define OBC_WATCHDOG_CODE   0x64



typedef enum{//-------------Typedefs and Enums-------------

    LONG_PAUSE,
    FIRST_PULSE,
    SHORT_PAUSE,
    SECOND_PULSE
}SYSTEM_HeartbeatState_t;

typedef enum{
    INIT        = 0x00,
    DETUMBLE    = 0x01,
    IDLE        = 0x02,
    EXPERIMENT  = 0x03,
    LINK        = 0x04,
    DEBUG       = 0x05
}SYSTEM_State_t;



typedef struct{
    SYSTEM_State_t state;
    SYSTEM_State_t previous_state;
    bool state_entry;
    SYSTEM_HeartbeatState_t stat_led_state;
    uint8_t heartbeat_tick_count;
    uint8_t watchdog_tick_count;
}SYSTEM_Handler_t;

extern SYSTEM_Handler_t hsys;




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