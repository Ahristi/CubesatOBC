#include "logging.h"

#include <Arduino.h>
#include <SD.h>
#include <Wire.h>



LOGGING_EPSTelemetry_t EPS_telemetry;
LOGGING_ADCSTelemetry_t ADCS_telemetry;
LOGGING_faults_t satellite_faults;
RTC_Time_t RTC_time;


void LOGGING_Init()
{
    pinMode(SD_CS_PIN, OUTPUT);
    pinMode(SD_SW_PIN, INPUT);
    if (!SD.begin(SD_CS_PIN))
    {
        Serial.println("SD card initialisation failed!");
        satellite_faults.OBC_Faults |= OBC_FAULT_DEAD_SD_CARD;
        if (digitalRead(SD_SW_PIN))
        {
            satellite_faults.OBC_Faults |= OBC_FAULT_NO_SD_CARD;
        }
    }
    Serial.println("SD Card Initialised!");
    Wire.beginTransmission(RV3028_ADDR);
    if (Wire.endTransmission() != 0)
    {
        satellite_faults.OBC_Faults |= OBC_FAULT_RTC;
        Serial.println("RV-3028 not detected");
    }
    Serial.println("RV-3028 detected");
}
bool LOGGING_getTime(RTC_Time_t *time)
{
    Wire.beginTransmission(RV3028_ADDR);
    Wire.write(0x00); 

    if (Wire.endTransmission(false) != 0)
    {
        satellite_faults.OBC_Faults |= OBC_FAULT_RTC;
        return false;
    }

    Wire.requestFrom(RV3028_ADDR, 7);

    if (Wire.available() < 7)
    {
        satellite_faults.OBC_Faults |= OBC_FAULT_RTC;
        return false;
    }

    time->seconds = bcdToDec(Wire.read() & 0x7F);
    time->minutes = bcdToDec(Wire.read() & 0x7F);
    time->hours   = bcdToDec(Wire.read() & 0x3F);

    Wire.read(); // Skip weekday register

    time->day     = bcdToDec(Wire.read() & 0x3F);
    time->month   = bcdToDec(Wire.read() & 0x1F);
    time->year    = 2000 + bcdToDec(Wire.read());

    return true;
}


void LOGGING_task()
{
    RTC_Time_t rtc_time;

    if (LOGGING_getTime(&rtc_time))
    {
        Serial.printf(
            "%04u-%02u-%02u %02u:%02u:%02u UTC\n",
            rtc_time.year,
            rtc_time.month,
            rtc_time.day,
            rtc_time.hours,
            rtc_time.minutes,
            rtc_time.seconds
        );
    }


    else
    {
        Serial.println("RTC read failed");
    }
    char filename[32];
    snprintf(filename, sizeof(filename),
             "/%04u%02u%02u.csv",
             rtc_time.year,
             rtc_time.month,
             rtc_time.day);
    File file = SD.open(filename, FILE_WRITE);
    if (!file)
    {
        Serial.println("Failed to open log file");
        satellite_faults.OBC_Faults |= OBC_FAULT_DEAD_SD_CARD;
        return;
    }

    file.printf(
            "%04u,%02u,%02u,%02u,%02u,%02u,"
            "%u,%u,%u,"
            "%u,%u,%u,"
            "%u,%u,"
            "%u,%u,%u,"
            "%u,%u,%u,%u,"
            "%u,%u,%u,%u,"
            "%d,%d,%d,"
            "%d,%d,%d,"
            "%d,%d,%d,"
            "0x%04X\n",
        rtc_time.year,
        rtc_time.month,
        rtc_time.day,
        rtc_time.hours,
        rtc_time.minutes,
        rtc_time.seconds,
        EPS_telemetry.rail_3v3.voltage,
        EPS_telemetry.rail_3v3.current_ch1,
        EPS_telemetry.rail_3v3.current_ch2,

        EPS_telemetry.rail_5v.voltage,
        EPS_telemetry.rail_5v.current_ch1,
        EPS_telemetry.rail_5v.current_ch2,

        EPS_telemetry.rail_6v.voltage,
        EPS_telemetry.rail_6v.current_ch1,

        EPS_telemetry.rail_12v.voltage,
        EPS_telemetry.rail_12v.current_ch1,
        EPS_telemetry.rail_12v.current_ch2,

        EPS_telemetry.mppt1_voltage,
        EPS_telemetry.mppt1_current,
        EPS_telemetry.mppt2_voltage,
        EPS_telemetry.mppt2_current,

        EPS_telemetry.battery_voltage,
        EPS_telemetry.battery_current,
        EPS_telemetry.battery_temp,
        EPS_telemetry.mcu_temp,

        ADCS_telemetry.roll,
        ADCS_telemetry.pitch,
        ADCS_telemetry.yaw,
        ADCS_telemetry.x_rw_speed,
        ADCS_telemetry.y_rw_speed,
        ADCS_telemetry.z_rw_sped,
        ADCS_telemetry.x_mag_current,
        ADCS_telemetry.y_mag_current,
        ADCS_telemetry.z_mag_current,

        satellite_faults.OBC_Faults
    );
    file.close();
}



uint8_t decToBcd(uint8_t val)
{
    return ((val / 10) << 4) | (val % 10);
}

uint8_t bcdToDec(uint8_t val)
{
    return ((val >> 4) * 10) + (val & 0x0F);
}