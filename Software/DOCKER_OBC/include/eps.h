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


#define LEN_RAIL_TELEM 6
#define LEN_BMS_TELEM  8
#define LEN_SYS_TELEM  3


void EPS_task(void);
void EPS_RailTelemHandle(RailTelemetry_t* railTelem, uint8_t* buf, uint8_t len);
void EPS_BMSTelemHandle(uint8_t* buf, uint8_t len);
void EPS_SYSTelemHandle(uint8_t* buf, uint8_t len);
void EPS_printRailTelemetry(const char* name, const RailTelemetry_t* rail);
void EPS_debugPrint(void);
#endif