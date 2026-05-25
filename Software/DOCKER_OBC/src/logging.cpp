#include "logging.h"

#include <Arduino.h>
#include <SD.h>
#include <Wire.h>



LOGGING_EPSTelemetry_t EPS_telemetry;
LOGGING_ADCSTelemetry_t ADCS_telemetry;
LOGGING_faults_t satellite_faults;
RTC_Time_t RTC_time;
LOGGING_Metadata_t wod_meta;

void LOGGING_Init()
{
    pinMode(SD_CS_PIN, OUTPUT);
    pinMode(SD_SW_PIN, INPUT);

    //Check SD Card functionality
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

    //Check the RTC
    Wire.beginTransmission(RV3028_ADDR);
    if (Wire.endTransmission() != 0)
    {
        satellite_faults.OBC_Faults |= OBC_FAULT_RTC;
        Serial.println("RV-3028 not detected");
    }
    Serial.println("RV-3028 detected");

    //Initialise metadata for WOD file, experiment file and results file

    //Check for WOD metadata, if it doesn't exist then create ite
    if (SD.exists(WOD_META_FILE))
    {
        if (!LOGGING_readMetadata(WOD_META_FILE, &wod_meta))
        {
            satellite_faults.OBC_Faults |= OBC_FAULT_DEAD_SD_CARD;
            return;
        }
        Serial.println("Successfully read WOD Metadata.");
        //check the wod struct to make sure it matches expected
        if ((wod_meta.ID != WOD_META_ID) || (wod_meta.max_records != WOD_MAX_RECORDS) || (wod_meta.record_size != sizeof(LOGGING_Record_t)))
        {
            Serial.println("Invalid metadata. Reinitialising.");
            wod_meta.ID = WOD_META_ID;
            wod_meta.max_records = WOD_MAX_RECORDS;
            wod_meta.record_size = sizeof(LOGGING_Record_t);
            wod_meta.read_ptr = 0;
            wod_meta.write_ptr = 0;
            if (!LOGGING_saveMetadata(WOD_META_FILE, &wod_meta))
            {
                satellite_faults.OBC_Faults |= OBC_FAULT_DEAD_SD_CARD;
            }
        }
    }
    else
    {
        Serial.println("WOD metadata doesn't exist. Creating new metadata file.");
        wod_meta.ID = WOD_META_ID;
        wod_meta.max_records = WOD_MAX_RECORDS;
        wod_meta.record_size = sizeof(LOGGING_Record_t);
        wod_meta.read_ptr = 0;
        wod_meta.write_ptr = 0;
        if (!LOGGING_saveMetadata(WOD_META_FILE, &wod_meta))
        {
            satellite_faults.OBC_Faults |= OBC_FAULT_DEAD_SD_CARD;
        }
    }

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
    if (LOGGING_getTime(&RTC_time))
    {
        Serial.printf(
            "%04u-%02u-%02u %02u:%02u:%02u UTC\n",
            RTC_time.year,
            RTC_time.month,
            RTC_time.day,
            RTC_time.hours,
            RTC_time.minutes,
            RTC_time.seconds
        );
    }
    else
    {
        Serial.println("RTC read failed");
        return;
    }

    File file = SD.open(WOD_DATA_FILE, FILE_WRITE);
    if (!file)
    {
        Serial.println("Failed to open log file");
        satellite_faults.OBC_Faults |= OBC_FAULT_DEAD_SD_CARD;
        return;
    }
    uint32_t write_ptr = file.size() / sizeof(LOGGING_Record_t);
    Serial.printf("Current WOD write pointer: %lu\n", write_ptr);
    Serial.printf("Current WOD read pointer: %lu\n", wod_meta.read_ptr);
    LOGGING_Record_t record = {0};

    record.year    = RTC_time.year;
    record.month   = RTC_time.month;
    record.day     = RTC_time.day;
    record.hours   = RTC_time.hours;
    record.minutes = RTC_time.minutes;
    record.seconds = RTC_time.seconds;

    record.rail_3v3_voltage     = EPS_telemetry.rail_3v3.voltage;
    record.rail_3v3_current_ch1 = EPS_telemetry.rail_3v3.current_ch1;
    record.rail_3v3_current_ch2 = EPS_telemetry.rail_3v3.current_ch2;

    record.rail_5v_voltage     = EPS_telemetry.rail_5v.voltage;
    record.rail_5v_current_ch1 = EPS_telemetry.rail_5v.current_ch1;
    record.rail_5v_current_ch2 = EPS_telemetry.rail_5v.current_ch2;

    record.rail_6v_voltage     = EPS_telemetry.rail_6v.voltage;
    record.rail_6v_current_ch1 = EPS_telemetry.rail_6v.current_ch1;

    record.rail_12v_voltage     = EPS_telemetry.rail_12v.voltage;
    record.rail_12v_current_ch1 = EPS_telemetry.rail_12v.current_ch1;
    record.rail_12v_current_ch2 = EPS_telemetry.rail_12v.current_ch2;

    record.mppt1_voltage = EPS_telemetry.mppt1_voltage;
    record.mppt1_current = EPS_telemetry.mppt1_current;
    record.mppt2_voltage = EPS_telemetry.mppt2_voltage;
    record.mppt2_current = EPS_telemetry.mppt2_current;

    record.battery_voltage = EPS_telemetry.battery_voltage;
    record.battery_current = EPS_telemetry.battery_current;
    record.battery_temp    = EPS_telemetry.battery_temp;
    record.mcu_temp        = EPS_telemetry.mcu_temp;

    record.roll  = ADCS_telemetry.roll;
    record.pitch = ADCS_telemetry.pitch;
    record.yaw   = ADCS_telemetry.yaw;

    record.x_rw_speed = ADCS_telemetry.x_rw_speed;
    record.y_rw_speed = ADCS_telemetry.y_rw_speed;
    record.z_rw_speed = ADCS_telemetry.z_rw_speed;

    record.x_mag_current = ADCS_telemetry.x_mag_current;
    record.y_mag_current = ADCS_telemetry.y_mag_current;
    record.z_mag_current = ADCS_telemetry.z_mag_current;

    record.obc_faults = satellite_faults.OBC_Faults;
    size_t bytes_written = file.write((uint8_t *)&record, sizeof(LOGGING_Record_t));
    wod_meta.write_ptr = write_ptr + 1;

    if (bytes_written != sizeof(LOGGING_Record_t))
    {
        Serial.println("Failed to write !LOGGING_readMetaDatacomplete log record");
        satellite_faults.OBC_Faults |= OBC_FAULT_DEAD_SD_CARD;
    }

    
}


bool LOGGING_readMetadata(const char *filename, const LOGGING_Metadata_t *meta)
{
    File file = SD.open(WOD_META_FILE, FILE_READ);

    if (!file)
    {
        Serial.println("Failed to open metadata file");
        return false;
    }

    size_t bytes_read = file.read((uint8_t *)&wod_meta,
                                sizeof(LOGGING_Metadata_t));

    file.close();

    if (bytes_read != sizeof(LOGGING_Metadata_t))
    {
        Serial.println("Metadata read incomplete");
        return false;
    }
    return true;
}
bool LOGGING_saveMetadata(const char *filename, const LOGGING_Metadata_t *meta)
{
    File file = SD.open(filename, FILE_WRITE);
    if (!file)
    {
        Serial.println("Failed to open metadata");
        return false;
    }

    if (!file.seek(0))
    {
        Serial.println("Failed to seek metadata file");
        file.close();
        return false;
    }
    size_t written = file.write((const uint8_t *)meta, sizeof(LOGGING_Metadata_t));
    file.close();

    if (written != sizeof(LOGGING_Metadata_t))
    {
        Serial.println("Failed to complete metadata");
    }
    return true;
}
bool LOGGING_readWODRecord(uint32_t pointer, LOGGING_Record_t *record)
{
    if (record == NULL)
    {
        return false;
    }

    File file = SD.open(WOD_DATA_FILE, FILE_READ);
    if (!file)
    {
        satellite_faults.OBC_Faults |= OBC_FAULT_DEAD_SD_CARD;
        return false;
    }

    uint32_t offset = pointer * sizeof(LOGGING_Record_t);

    if (!file.seek(offset))
    {
        file.close();
        return false;
    }

    size_t bytes_read = file.read((uint8_t *)record, sizeof(LOGGING_Record_t));

    file.close();

    if (bytes_read != sizeof(LOGGING_Record_t))
    {
        return false;
    }

    return true;
}








uint8_t decToBcd(uint8_t val)
{
    return ((val / 10) << 4) | (val % 10);
}

uint8_t bcdToDec(uint8_t val)
{
    return ((val >> 4) * 10) + (val & 0x0F);
}