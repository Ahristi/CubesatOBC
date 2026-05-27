#include "comms.h"
#include <Arduino.h>


COMMS_Handler_t hcomms;

void COMMS_Init(void)
{
    Serial3.begin(COMMS_BAUDRATE);
    Serial.println("COMMS UART initialised on Serial3");
    hcomms.beacon_tick = 0;
    hcomms.wod_handler.state = DOWNLINK_IDLE;
    hcomms.wod_handler.ack_retries     = 0;
    hcomms.wod_handler.downlink_active = false;
}

void COMMS_task()
{
    COMMS_wodDownlinkHandler();
    if (!hcomms.wod_handler.downlink_active)
    {
        hcomms.beacon_tick++;
        if (hcomms.beacon_tick >= BEACON_TICK_OC)
        {
            hcomms.beacon_tick = 0;
            COMMS_sendBeacon();
        }
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
    data->omega_x               = ADCS_telemetry.omega_x;    
    data->omega_y               = ADCS_telemetry.omega_y;
    data->omega_z               = ADCS_telemetry.omega_z;    
    data->x_mag_current         = ADCS_telemetry.x_mag_current;
    data->y_mag_current         = ADCS_telemetry.y_mag_current;
    data->z_mag_current         = ADCS_telemetry.z_mag_current;
    data->EPS_Faults            = satellite_faults.EPS_Faults;
    data->OBC_Faults            = satellite_faults.OBC_Faults;
    data->ADCS_Faults           = satellite_faults.ADCS_Faults;
    data->Payload_Faults        = satellite_faults.Payload_Faults;
    data->Comms_Faults          = satellite_faults.Comms_Faults;
}


void COMMS_wodDownlinkHandler(void)
{
    switch (hcomms.wod_handler.state)
    {
        case DOWNLINK_IDLE:
        {
            if (COMMS_getLink())
            {
                Serial.println("Link Received!");
                hcomms.wod_handler.state = DOWNLINK_SEND_INFO;
                hcomms.wod_handler.end_ptr = wod_meta.write_ptr;
                hcomms.wod_handler.num_chunks = hcomms.wod_handler.end_ptr - wod_meta.read_ptr;
                if (hcomms.wod_handler.num_chunks == 0)
                {
                    hcomms.wod_handler.state = DOWNLINK_COMPLETE;
                }
                else
                {
                    hcomms.wod_handler.state = DOWNLINK_SEND_INFO;
                }
            }
            break;
        }
        case DOWNLINK_SEND_INFO:
        {
            //Note: The uart message uses the same ID that is sent in the payload from the WOD_INFO_ID define.
            //I know its a little confusing, but that ID is removed in the uart protocol wrapper, so we also add it in the data payload
            //so that the ground station knows what we are sending it. 
            COMMS_sendFileInfo(WOD_INFO_ID, sizeof(LOGGING_Record_t), hcomms.wod_handler.num_chunks);
            hcomms.wod_handler.state = DOWNLINK_WAIT_ACK;
            break;
        }
        case DOWNLINK_SEND_CHUNK:
        {
            COMMS_sendWOD();
            hcomms.wod_handler.state = DOWNLINK_WAIT_ACK;
            break;
        }
        case DOWNLINK_WAIT_ACK:
        {
            if (COMMS_getAck())
            {
                hcomms.wod_handler.ack_retries    = 0;
                wod_meta.read_ptr++;
                if (wod_meta.read_ptr > hcomms.wod_handler.end_ptr)
                {
                    hcomms.wod_handler.state = DOWNLINK_COMPLETE; 
                }
                else
                {
                    hcomms.wod_handler.state = DOWNLINK_SEND_CHUNK;
                }
            }
            else
            {
                hcomms.wod_handler.ack_retries++;
                if (hcomms.wod_handler.ack_retries >= MAX_ACK_RETRIES)
                {
                    hcomms.wod_handler.ack_retries    = 0;
                    hcomms.wod_handler.downlink_active = false;
                    hcomms.wod_handler.state = DOWNLINK_ERROR;
                }
            }
            break;
        }
        case DOWNLINK_COMPLETE:
        {
            //Save the new read pointer to the SD card so we don't resend old data.
            LOGGING_saveMetadata(WOD_META_FILE, &wod_meta);
            COMMS_sendEndTransfer();
            hcomms.wod_handler.downlink_active = false;
            hcomms.wod_handler.state = DOWNLINK_IDLE;
            break;
        }
        case DOWNLINK_ERROR:
        {
            hcomms.wod_handler.state = DOWNLINK_IDLE;
            break;
        }
        default:
        {
            hcomms.wod_handler.state = DOWNLINK_IDLE;
            break;
        }
    }
}


bool COMMS_getLink(void)
{
    UART_msg_t msg;
    if (UART_receive(&Serial3, &msg, DEFAULT_UART_TIMEOUT_US))
    {
        Serial.println("Received message from comms board");
        if (msg.length < 1)
        {
            Serial.println("Bad comms request length");
            return false;
        }

        if ((msg.id == WOD_REQUEST_ID))
        {
            hcomms.wod_handler.downlink_active = true;
            return true;
        }
        else
        {
            Serial.println("Warning: Bad command received from Comms board.");
            return false;
        }
    }
    return false;
}



bool COMMS_getAck(void)
{
    UART_msg_t msg;
    if (UART_receive(&Serial3, &msg, DEFAULT_UART_TIMEOUT_US))
    {
        if (msg.length < 1)
        {
            Serial.println("Bad ack message length");
            return false;
        }

        if ((msg.id == COMMS_ACK_ID))
        {
            return true;
        }
        else
        {
            Serial.println("Warning: Bad command received from Comms board.");
            return false;
        }
    }
    return false;
}

void COMMS_sendFileInfo(uint8_t fileID, uint32_t chunk_size, uint32_t num_chunks)
{
    UART_msg_t msg;
    uint8_t data[WOD_INFO_BYTES];

    msg.sof    = UART_SOF;
    msg.id     = WOD_INFO_ID;
    msg.length = WOD_INFO_BYTES;

    data[0] = fileID;

    memcpy(&data[1], &chunk_size, sizeof(uint32_t));
    memcpy(&data[5], &num_chunks, sizeof(uint32_t));

    memcpy(msg.payload, data, WOD_INFO_BYTES);

    UART_transmit(&Serial3, &msg);

    Serial.println("Sent WOD INFO");
}

bool COMMS_sendWOD(void)
{
    UART_msg_t msg;
    LOGGING_Record_t record;
    if(!LOGGING_readWODRecord(wod_meta.read_ptr, &record))
    {
        return false;
    }
    msg.sof = UART_SOF;
    msg.id  = WOD_RECORD_ID;
    msg.length = sizeof(LOGGING_Record_t);
    memcpy(msg.payload, &record, sizeof(LOGGING_Record_t));
    UART_transmit(&Serial3, &msg);
    return true;
}
bool COMMS_sendEndTransfer(void)
{
    UART_msg_t msg;
    msg.sof = UART_SOF;
    msg.id  = END_TRANSFER_ID;
    msg.length = 1;
    msg.payload[0] = END_TRANSFER_ID; //Send end transfer ID in the payload as well.
    UART_transmit(&Serial3, &msg);
    return true;
}
