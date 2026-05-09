#ifndef LOGGING_H
#define LOGGING_H
#include <stdint.h>

//-------------Defines-------------
#define SD_CS_PIN 10
#define SD_SW_PIN 2
#define RV3028_ADDR 0x52

//-------------Bitmasks-------------
#define OBC_FAULT_NO_SD_CARD (1U)   << 0
#define OBC_FAULT_DEAD_SD_CARD (1U) << 1
#define OBC_FAULT_RTC (1U)          << 2



//-------------Typedefs and Enums-------------
typedef struct
{
    uint16_t voltage;
    uint16_t current_ch1;
    uint16_t current_ch2;
} RailTelemetry_t;



typedef struct{
    RailTelemetry_t rail_3v3;
    RailTelemetry_t rail_5v;
    RailTelemetry_t rail_6v;
    RailTelemetry_t rail_12v;


    uint16_t battery_voltage;
    uint16_t sys_voltage;
    uint16_t battery_current;
    uint8_t battery_temp;

    uint16_t mcu_temp;
    uint8_t  charger_die_temp;

    uint16_t mppt1_voltage;
    uint16_t mppt2_voltage;
    uint16_t mppt1_current;
    uint16_t mppt2_current;

    uint8_t eFuse_states;
    uint8_t eFuse_faults;
}LOGGING_EPSTelemetry_t;

typedef struct{
    uint16_t roll;
    uint16_t pitch;
    uint16_t yaw;
    uint16_t x_rw_speed;
    uint16_t y_rw_speed;
    uint16_t z_rw_sped;

    uint16_t x_mag_current;
    uint16_t y_mag_current;
    uint16_t z_mag_current;

}LOGGING_ADCSTelemetry_t;

typedef struct{
    uint16_t EPS_Faults;
    uint16_t OBC_Faults;
    uint16_t ADCS_Faults;
    uint16_t Payload_Faults;
    uint16_t Comms_Faults;
}LOGGING_faults_t;

typedef struct
{
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} RTC_Time_t;



//-------------Variables-------------
extern LOGGING_EPSTelemetry_t EPS_telemetry;
extern LOGGING_ADCSTelemetry_t ADCS_telemetry;
extern LOGGING_faults_t satellite_faults;
extern RTC_Time_t RTC_time;


//-------------Function Prototypes-------------
void LOGGING_Init();
void LOGGING_task();
bool LOGGING_getTime(RTC_Time_t *time);
uint8_t decToBcd(uint8_t val);
uint8_t bcdToDec(uint8_t val);


#endif