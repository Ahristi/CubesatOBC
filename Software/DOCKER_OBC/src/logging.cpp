#include "logging.h"

LOGGING_EPSTelemetry_t EPS_telemetry;
LOGGING_ADCSTelemetry_t ADCS_telemetry;
LOGGING_faults_t satellite_faults;
RTC_Time_t RTC_time;
FILE_Handler_t wod = {
    .metadata_file_name = WOD_META_FILE_NAME,
    .file_name          = WOD_DATA_FILE_NAME,
    .metadata = {
        .ID         = WOD_META_ID,
        .chunk_size = sizeof(LOGGING_Record_t),
        .num_chunks = 0,
        .max_chunks = WOD_MAX_RECORDS,
        .read_ptr   = 0
    }
};


void LOGGING_Init()
{   
    //Check the RTC
    Wire.beginTransmission(RV3028_ADDR);
    if (Wire.endTransmission() != 0)
    {
        satellite_faults.OBC_Faults |= OBC_FAULT_RTC;
        Serial.println("RV-3028 not detected");
    }
    Serial.println("RV-3028 detected");
    FILE_initialiseMetadata(&wod);
    Serial.println("Metadata initialised with num chunks: " + String(wod.metadata.num_chunks));

    FILE_open(&wod, FILE_OPEN_FOR_WRITE);
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
    if (hcomms.state == COMMS_WOD_DOWNLINK)
    {
        return;
    }
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

    record.mppt1_voltage   = EPS_telemetry.mppt1_voltage;
    record.mppt1_current   = EPS_telemetry.mppt1_current;
    record.mppt2_voltage   = EPS_telemetry.mppt2_voltage;
    record.mppt2_current   = EPS_telemetry.mppt2_current;

    record.battery_voltage = EPS_telemetry.battery_voltage;
    record.battery_current = EPS_telemetry.battery_current;
    record.battery_temp    = EPS_telemetry.battery_temp;
    record.mcu_temp        = EPS_telemetry.mcu_temp;

    record.roll            = ADCS_telemetry.roll;
    record.pitch           = ADCS_telemetry.pitch;
    record.yaw             = ADCS_telemetry.yaw;

    record.omega_x         = ADCS_telemetry.omega_x;
    record.omega_y         = ADCS_telemetry.omega_y;
    record.omega_z         = ADCS_telemetry.omega_z;

    record.x_rw_speed      = ADCS_telemetry.x_rw_speed;
    record.y_rw_speed      = ADCS_telemetry.y_rw_speed;
    record.z_rw_speed      = ADCS_telemetry.z_rw_speed; 

    record.x_mag_current   = ADCS_telemetry.x_mag_current;
    record.y_mag_current   = ADCS_telemetry.y_mag_current;
    record.z_mag_current   = ADCS_telemetry.z_mag_current;

    record.detumble_scale  = ADCS_telemetry.detumble_scale;

    record.OBC_Faults      = satellite_faults.OBC_Faults;
    record.ADCS_Faults     = satellite_faults.ADCS_Faults;
    record.EPS_Faults      = satellite_faults.EPS_Faults;
    record.Payload_Faults  = satellite_faults.Payload_Faults;
    record.Comms_Faults    = satellite_faults.Comms_Faults;


    //Note: FILE_write increments the chunk counter
    size_t bytes_written = FILE_write(&wod, (uint8_t*)&record, sizeof(LOGGING_Record_t));

    if (bytes_written == sizeof(record))
    {
        FILE_writeMetadata(&wod);
    }
    else
    {
        satellite_faults.OBC_Faults |= OBC_FAULT_DEAD_SD_CARD;
    }
    Serial.println("Read ptr: " + String(wod.metadata.read_ptr));
}



uint8_t decToBcd(uint8_t val)
{
    return ((val / 10) << 4) | (val % 10);
}

uint8_t bcdToDec(uint8_t val)
{
    return ((val >> 4) * 10) + (val & 0x0F);
}