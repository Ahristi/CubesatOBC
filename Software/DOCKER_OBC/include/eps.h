#ifndef EPS_H
#define EPS_H

#include "can.h"
#include "logging.h"

//-------------Defines-------------
#define EPS_EFUSE_CONTROL_CMD_ID        0x47
#define EPS_CONVERTER_CONTROL_CMD_ID    0x65
#define EPS_WATCHDOG_ID                 0x69

#define EPS_3V3_TELEMETRY_ID            0x320
#define EPS_5V_TELEMETRY_ID             0x321
#define EPS_6V_TELEMETRY_ID             0x322
#define EPS_12V_TELEMETRY_ID            0x323
#define EPS_BMS_TELEMETRY_ID            0x324
#define EPS_SYS_TELEMETRY_ID            0x325
#define EPS_MPPT_TELEMETRY_ID           0x326

#define MASK_3V3_CH2_EN (1U)<<0
#define MASK_5V_CH1_EN  (1U)<<1
#define MASK_5V_CH2_EN  (1U)<<2
#define MASK_6V_CH1_EN  (1U)<<3
#define MASK_12V_CH1_EN (1U)<<4
#define MASK_12V_CH2_EN (1U)<<5

#define LEN_RAIL_TELEM 6
#define LEN_BMS_TELEM  8
#define LEN_MPPT_TELEM 8
#define LEN_SYS_TELEM  3
//-------------Typedef and Enums-------------
typedef struct{
    uint8_t eFuse_states;
    bool eFuse_msg_ready;
    
    uint8_t converter_states;
    bool converter_msg_ready;
}EPS_Handler_t;

typedef enum {
    EPS_EFUSE_3V3_CH2 = MASK_3V3_CH2_EN,
    EPS_EFUSE_5V_CH1  = MASK_5V_CH1_EN,
    EPS_EFUSE_5V_CH2  = MASK_5V_CH2_EN,
    EPS_EFUSE_6V_CH1  = MASK_6V_CH1_EN,
    EPS_EFUSE_12V_CH1 = MASK_12V_CH1_EN,
    EPS_EFUSE_12V_CH2 = MASK_12V_CH2_EN
} EPS_EFuse_t;



//-------------Variables-------------
extern EPS_Handler_t heps;

//-------------Function Prototypes-------------
void EPS_Init(void);
void EPS_task(void);
void EPS_RailTelemHandle(RailTelemetry_t* railTelem, uint8_t* buf, uint8_t len);
void EPS_BMSTelemHandle(uint8_t* buf, uint8_t len);
void EPS_SYSTelemHandle(uint8_t* buf, uint8_t len);
void EPS_MPPTTelemHandle(uint8_t* buf, uint8_t len);
void EPS_printRailTelemetry(const char* name, const RailTelemetry_t* rail);
void EPS_debugPrint(void);
void EPS_updateEfuses(void);
void EPS_enableEFuse(EPS_EFuse_t efuse);
void EPS_disableEFuse(EPS_EFuse_t efuse);


#endif