#include "comms.h"
#include <Arduino.h>


COMMS_Handler_t hcomms;

void COMMS_Init(void)
{
    Serial3.begin(COMMS_BAUDRATE);
    Serial.println("COMMS UART initialised on Serial3");
}

void COMMS_task()
{
    hcomms.beacon_tick++;
    
    if (hcomms.beacon_tick >= BEACON_TICK_OC)
    {
        hcomms.beacon_tick = 0;
        COMMS_sendBeacon();
    }

    return;
}


void COMMS_sendBeacon(void)
{
    UART_msg_t msg;
    msg.sof = UART_SOF;
    msg.id  = BEACON_MSG_ID;
    msg.length = BEACON_TIME_STRING_BYTES + CUBESAT_IDENTIFIER_BYTES + BEACON_MSG_DATA_BYTES;
    COMMS_BeaconData_t data;
    COMMS_packBeacon(&data);
    memcpy(msg.payload, &data, sizeof(COMMS_BeaconData_t));
    UART_transmit(&Serial3, &msg);
    Serial.println("Sent Beacon");
}

void COMMS_packBeacon(COMMS_BeaconData_t* data)
{
    snprintf(
        data->utc_time,
        BEACON_TIME_STRING_BYTES,
        "%04u-%02u-%02u %02u:%02u:%02u UTC",
        RTC_time.year,
        RTC_time.month,
        RTC_time.day,
        RTC_time.hours,
        RTC_time.minutes,
        RTC_time.seconds
    );

    memcpy(data->identifier, "DOCKER", CUBESAT_IDENTIFIER_BYTES);
    data->rail_3v3_voltage      = EPS_telemetry.rail_3v3.voltage;
    data->rail_3v3_current_ch1  = EPS_telemetry.rail_3v3.current_ch1;
    data->rail_3v3_current_ch2  = EPS_telemetry.rail_3v3.current_ch2;
    data->rail_5v_voltage       = EPS_telemetry.rail_5v.voltage;
    data->rail_5v_current_ch1   = EPS_telemetry.rail_5v.current_ch1;
    data->rail_5v_current_ch2   = EPS_telemetry.rail_5v.current_ch2;
    data->rail_6v_voltage       = EPS_telemetry.rail_6v.voltage;
    data->rail_6v_current_ch1   = EPS_telemetry.rail_6v.current_ch1; 
    data->rail_12v_voltage      = EPS_telemetry.rail_12v.voltage;
    data->rail_12v_current_ch1  = EPS_telemetry.rail_12v.current_ch1;
    data->rail_12v_current_ch2  = EPS_telemetry.rail_12v.current_ch2;
    data->battery_voltage       = EPS_telemetry.battery_voltage;
    data->sys_voltage           = EPS_telemetry.sys_voltage;
    data->battery_current       = EPS_telemetry.battery_current;
    data->battery_temp          = EPS_telemetry.battery_temp;
    data->mcu_temp              = EPS_telemetry.mcu_temp;
    data->charger_die_temp      = EPS_telemetry.charger_die_temp;
    data->mppt1_voltage         = EPS_telemetry.mppt1_voltage;
    data->mppt2_voltage         = EPS_telemetry.mppt2_voltage;
    data->mppt1_current         = EPS_telemetry.mppt1_current;
    data->mppt2_current         = EPS_telemetry.mppt2_current;
    data->eFuse_states          = EPS_telemetry.eFuse_states;
    data->eFuse_faults          = EPS_telemetry.eFuse_faults;
    data->roll                  = ADCS_telemetry.roll;
    data->pitch                 = ADCS_telemetry.pitch;
    data->yaw                   = ADCS_telemetry.yaw;
    data->x_rw_speed            = ADCS_telemetry.x_rw_speed;
    data->y_rw_speed            = ADCS_telemetry.y_rw_speed;
    data->z_rw_speed            = ADCS_telemetry.z_rw_speed;
    data->x_mag_current         = ADCS_telemetry.x_mag_current;
    data->y_mag_current         = ADCS_telemetry.y_mag_current;
    data->z_mag_current         = ADCS_telemetry.z_mag_current;
    data->EPS_Faults            = satellite_faults.EPS_Faults;
    data->OBC_Faults            = satellite_faults.OBC_Faults;
    data->ADCS_Faults           = satellite_faults.ADCS_Faults;
    data->Payload_Faults        = satellite_faults.Payload_Faults;
    data->Comms_Faults          = satellite_faults.Comms_Faults;
}

void COMMS_downLinkHandler(void)
{
    switch (hcomms.downlink_state)
    {
        case DOWNLINK_IDLE:
        {
            break;
        }
        case DOWNLINK_SEND_INFO:
        {
            COMMS_sendFileInfo();
            break;
        }
        case DOWNLINK_SEND_CHUNK:
        {
            COMMS_sendNextFileChunk();
            break;
        }
        case DOWNLINK_WAIT_ACK:
        {
            if (COMMS_getAck())
            {
                hcomms.ack_retries    = 0;
                hcomms.downlink_state = DOWNLINK_SEND_CHUNK;
            }
            else
            {
                hcomms.ack_retries++;
                if (hcomms.ack_retries >= MAX_ACK_RETRIES)
                {
                    hcomms.ack_retries    = 0;
                    hcomms.downlink_ready = 0;
                    hcomms.downlink_state = DOWNLINK_ERROR;
                }
            }
            break;
        }
        case DOWNLINK_COMPLETE:
        {
            COMMS_sendEndFile();
            break;
        }
        case DOWNLINK_ERROR:
        {
            break;
        }
        default:
        {
            hcomms.downlink_state = DOWNLINK_IDLE;
            break;
        }
            
    }
}