#include "comms.h"
#include <Arduino.h>

COMMS_Handler_t hcomms;
COMMS_downlinkHandler_t hdownlink_wod;
COMMS_downlinkHandler_t hdownlink_results;
COMMS_uplinkHandler_t huplink_experiment;


void COMMS_Init(void)
{
    Serial3.begin(COMMS_BAUDRATE);
    Serial.println("COMMS UART initialised on Serial3");
    hcomms.beacon_tick = 0;
    hcomms.state = COMMS_IDLE;
    hdownlink_wod.state = DOWNLINK_IDLE;
    hdownlink_wod.prev_state = DOWNLINK_IDLE;
}

void COMMS_task()
{
    switch (hcomms.state)
    {
        case COMMS_IDLE:
        {
            if (!COMMS_getLink(&hcomms))
            {
                hcomms.beacon_tick++;
                if (hcomms.beacon_tick >= BEACON_TICK_OC)
                {
                    hcomms.beacon_tick = 0;
                    COMMS_sendBeacon();
                }
                break;
            }
        }
        case COMMS_WOD_DOWNLINK:
        {
            COMMS_downlink(&wod, &hdownlink_wod);
            break;
        }
        case COMMS_EXPERIMENT_UPLINK:
        {
            COMMS_uplink(&hpayload.experiment_file, &huplink_experiment);
            break;
        }
        default:
        {
            hcomms.state = COMMS_IDLE;
            break;
        }

    }
    return;
}


void COMMS_sendBeacon(void)
{
    UART_msg_t msg;
    msg.sof = UART_SOF;
    msg.id  = BEACON_MSG_ID;
    msg.length = sizeof(COMMS_BeaconData_t);
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
    data->x_mag_field_sense     = ADCS_telemetry.x_mag_field_sense;    
    data->y_mag_field_sense     = ADCS_telemetry.y_mag_field_sense;
    data->z_mag_field_sense     = ADCS_telemetry.z_mag_field_sense;    
    data->x_mag_field_filt      = ADCS_telemetry.x_mag_field_filt;
    data->y_mag_field_filt      = ADCS_telemetry.y_mag_field_filt;
    data->z_mag_field_filt      = ADCS_telemetry.z_mag_field_filt;
    data->EPS_Faults            = satellite_faults.EPS_Faults;
    data->OBC_Faults            = satellite_faults.OBC_Faults;
    data->ADCS_Faults           = satellite_faults.ADCS_Faults;
    data->Payload_Faults        = satellite_faults.Payload_Faults;
    data->Comms_Faults          = satellite_faults.Comms_Faults;
}


bool COMMS_getLink(COMMS_Handler_t* hcomms)
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
            hcomms->state = COMMS_WOD_DOWNLINK;
            return true;
        }
        else if (msg.id == EXPERIMENT_COMMAND_ID)
        {
            Serial.println("Experiment Uplink begin");
            hcomms->state = COMMS_EXPERIMENT_UPLINK;
            return true;
        }
        else if (msg.id == RESULT_REQUEST_ID)
        {
            hcomms->state = COMMS_RESULTS_DOWNLINK;
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
bool COMMS_sendAck(void)
{
    UART_msg_t msg = {0};

    msg.sof    = UART_SOF;
    msg.id     = COMMS_ACK_ID;
    msg.length = 1;
    msg.payload[0] = COMMS_ACK_ID;

    UART_transmit(&Serial3, &msg);

    return true;
}


bool COMMS_sendEndTransfer(void)
{
    UART_msg_t msg = {0};

    msg.sof = UART_SOF;
    msg.id  = END_TRANSFER_ID;
    msg.length = 1;
    msg.payload[0] = END_TRANSFER_ID; // Send end transfer ID in payload as well.

    UART_transmit(&Serial3, &msg);
    return true;
}

bool COMMS_getEndTransfer(void)
{
    UART_msg_t msg = {0};
    if (UART_receive(&Serial3, &msg, DEFAULT_UART_TIMEOUT_US))
    {
        if (msg.id != END_TRANSFER_ID)
        {
            Serial.println("Warning: Expected END_TRANSFER but received different message ID.");
            return false;
        }
        if (msg.length < 1)
        {
            Serial.println("Bad END_TRANSFER message length.");
            return false;
        }
        if (msg.payload[0] != END_TRANSFER_ID)
        {
            Serial.println("Bad END_TRANSFER payload.");
            return false;
        }
        return true;
    }
    return false;
}

//-----------BEGIN ABSTRACTED COMMS-----------



void COMMS_downlink(FILE_Handler_t* hfile, COMMS_downlinkHandler_t* hdownlink)
{
    switch (hdownlink->state)
    {
        case DOWNLINK_IDLE:
        {
            if (hfile->metadata.read_ptr == hfile->metadata.num_chunks)
            {
                hdownlink->prev_state = DOWNLINK_IDLE;
                hdownlink->state = DOWNLINK_COMPLETE;
            }
            else
            { 
                hdownlink->prev_state = DOWNLINK_IDLE;
                hdownlink->state = DOWNLINK_SEND_INFO;
            }
            break;
        }
        case DOWNLINK_SEND_INFO:
        {
            uint32_t num_packets = hfile->metadata.num_chunks -  hfile->metadata.read_ptr;
            COMMS_sendFileInfo(hfile->metadata.ID,  hfile->metadata.chunk_size, num_packets);
         
            hdownlink->prev_state = DOWNLINK_SEND_INFO;
            hdownlink->state = DOWNLINK_WAIT_ACK;
            break;
        }
        case DOWNLINK_SEND_CHUNK:
        {
            uint8_t chunk_size = hfile->metadata.chunk_size;
            uint8_t chunk[MAX_CHUNK_SIZE];

            if (chunk_size > MAX_CHUNK_SIZE)
            {
                Serial.println("ERROR: Chunk exceeds max chunk size");
                hdownlink->state = DOWNLINK_ERROR;
                break;
            }

            size_t bytes_read = FILE_read(hfile, chunk);

            if (bytes_read == 0 || bytes_read > UINT8_MAX)
            {
                Serial.println("ERROR: No bytes read");
                hdownlink->state = DOWNLINK_ERROR;
                break;
            }

            COMMS_sendPacket(CHUNK_ID, chunk, (uint8_t)bytes_read);

            Serial.println("Sent chunk");
            Serial.print("Read Pointer: ");
            Serial.println(hfile->metadata.read_ptr);

            hdownlink->prev_state = DOWNLINK_SEND_CHUNK;
            hdownlink->state = DOWNLINK_WAIT_ACK;
            break;
        }
        case DOWNLINK_WAIT_ACK:
        {
            if (COMMS_getAck())
            {
                Serial.println("ACK received");
                hdownlink->ack_retries = 0;

                if (hdownlink->prev_state == DOWNLINK_SEND_INFO)
                {
                    hdownlink->prev_state = DOWNLINK_WAIT_ACK;
                    hdownlink->state = DOWNLINK_SEND_CHUNK;
                }
                else if (hdownlink->prev_state == DOWNLINK_SEND_CHUNK)
                {
                    hfile->metadata.read_ptr++;

                    if (hfile->metadata.read_ptr >= hfile->metadata.num_chunks)
                    {
                        hdownlink->prev_state = DOWNLINK_WAIT_ACK;
                        hdownlink->state = DOWNLINK_COMPLETE;
                    }
                    else
                    {
                        hdownlink->prev_state = DOWNLINK_WAIT_ACK;
                        hdownlink->state = DOWNLINK_SEND_CHUNK;
                    }
                }
                else if (hdownlink->prev_state == DOWNLINK_COMPLETE)
                {
                        hdownlink->prev_state = DOWNLINK_WAIT_ACK;
                        hdownlink->state = DOWNLINK_IDLE;
                        hcomms.state = COMMS_IDLE;
                }
            }
            else
            {
                Serial.println("ACK not received");
                Serial.print("ACK retry: ");
                Serial.println(hdownlink->ack_retries);
                hdownlink->ack_retries++;
                if (hdownlink->ack_retries >= MAX_ACK_RETRIES)
                {
                    Serial.println("ERROR: Exceeded max retries");
                    hdownlink->ack_retries    = 0;
                    hdownlink->state = DOWNLINK_ERROR;
                }
                else
                {
                    hdownlink->state = hdownlink->prev_state;
                    hdownlink->prev_state = DOWNLINK_WAIT_ACK;
                }
            }
            break;
        }
        case DOWNLINK_COMPLETE:
        {
            //Save the new read pointer to the SD card so we don't resend old data.
            Serial.println("Downlink Complete");
            FILE_writeMetadata(hfile);
            FILE_close(hfile);
            FILE_open(hfile, FILE_OPEN_FOR_WRITE);
            COMMS_sendEndTransfer();
            hdownlink->prev_state = DOWNLINK_COMPLETE;
            hdownlink->state = DOWNLINK_WAIT_ACK;
            break;
        }
        case DOWNLINK_ERROR:
        {
            Serial.println("Downlink ERROR");
            FILE_writeMetadata(hfile);
            FILE_close(hfile);
            FILE_open(hfile, FILE_OPEN_FOR_WRITE);
            hdownlink->prev_state = DOWNLINK_ERROR;
            hdownlink->state = DOWNLINK_IDLE;
            hcomms.state = COMMS_IDLE;
            break;
        }
        default:
        {
            hdownlink->state = DOWNLINK_IDLE;
            hcomms.state = COMMS_IDLE;
            break;
        }
    }
}



void COMMS_uplink(FILE_Handler_t* hfile, COMMS_uplinkHandler_t* huplink)
{
    switch (huplink->state)
    {
        case UPLINK_IDLE:
        {
            if (!FILE_clear(hfile))
            {
                huplink->prev_state = UPLINK_IDLE;
                huplink->state = UPLINK_ERROR;
                break;
            }
            Serial.println(hfile->file_name);
            Serial.println("Successfully cleared file.");
            huplink->prev_state = UPLINK_IDLE;
            huplink->state = UPLINK_RECEIVE_INFO;
            break;
        }
        case UPLINK_RECEIVE_INFO:
        {
            if (COMMS_receiveFileInfo(hfile, huplink))
            {
                
                Serial.println("File info received");
                Serial.println("File chunks: ");
                Serial.println(huplink->file_chunks);
                if (!FILE_open(hfile, FILE_OPEN_FOR_WRITE))
                {
                    huplink->prev_state = UPLINK_RECEIVE_INFO;
                    huplink->state = UPLINK_ERROR;
                    break;
                }
                huplink->prev_state = UPLINK_RECEIVE_INFO;
                huplink->state = UPLINK_SEND_ACK;
            }
            else if (huplink->timeout_ctr >= UPLINK_TIMEOUT)
            {
                Serial.println("Error: Uplink timeout");
                huplink->timeout_ctr = 0;
                huplink->prev_state = UPLINK_RECEIVE_INFO;
                huplink->state = UPLINK_ERROR;
            }
            else
            {
                
                huplink->timeout_ctr++;
            }
            break;
        }
        case UPLINK_RECEIVE_PACKET:
        {
            Packet_t packet;
            if (COMMS_receivePacket(&packet))
            {
                huplink->timeout_ctr = 0;
                if (packet.packet_idx == hfile->metadata.num_chunks)
                {
                    //Packet matches write ptr
                    FILE_write(hfile, packet.payload, hfile->metadata.chunk_size);
                    Serial.println(hfile->metadata.num_chunks);
                    huplink->prev_state = UPLINK_RECEIVE_PACKET;
                    huplink->state = UPLINK_SEND_ACK;
                    break;
                }
                else if (packet.packet_idx < hfile->metadata.num_chunks)
                {
                    // Duplicate packet
                    huplink->prev_state = UPLINK_RECEIVE_PACKET;
                    huplink->state = UPLINK_SEND_ACK;
                    break;
                }
                else
                {
                    // missed a packet
                    Serial.println("ERROR: Packet Lost expected " + String(packet.packet_idx) +" expected " + String(hfile->metadata.num_chunks));
                    huplink->prev_state = UPLINK_RECEIVE_PACKET;
                    huplink->state = UPLINK_ERROR;
                    break;
                }
            }
            else 
            {
                huplink->timeout_ctr++;
                if (huplink->timeout_ctr >= UPLINK_TIMEOUT)
                {
                    Serial.println("Error: Uplink timeout during receive packet");
                    huplink->timeout_ctr = 0;
                    huplink->prev_state = UPLINK_RECEIVE_PACKET;
                    huplink->state = UPLINK_ERROR;
                    break;
                }
            }
            break;
        }
        case UPLINK_SEND_ACK:
        {
            Serial.println("Sending ACK");
            COMMS_sendAck();
            if (hfile->metadata.num_chunks == huplink->file_chunks)
            {
                huplink->prev_state = UPLINK_SEND_ACK;
                huplink->state = UPLINK_COMPLETE;
            }
            else if (huplink->prev_state == UPLINK_RECEIVE_PACKET)
            {
                huplink->state = UPLINK_RECEIVE_PACKET;
                huplink->prev_state = UPLINK_SEND_ACK;
            }
            else if (huplink->prev_state == UPLINK_RECEIVE_INFO)
            {
                huplink->state = UPLINK_RECEIVE_PACKET;
                huplink->prev_state = UPLINK_SEND_ACK;
            }
            else if (huplink->prev_state == UPLINK_COMPLETE)
            {
                huplink->prev_state = UPLINK_SEND_ACK;
                huplink->state = UPLINK_IDLE;
                hpayload.experiment_ready = true;
                hcomms.state = COMMS_IDLE;
            }
            else
            {
                huplink->state = UPLINK_ERROR;
                huplink->prev_state = UPLINK_SEND_ACK;
            }
            break;
        }
        case UPLINK_COMPLETE:
        {
            FILE_writeMetadata(hfile);
            if (COMMS_getEndTransfer())
            {
                Serial.println("Uplink Complete");
                huplink->prev_state = UPLINK_COMPLETE;
                huplink->state = UPLINK_SEND_ACK;
            }
            else if (huplink->timeout_ctr >= UPLINK_TIMEOUT)
            {
                huplink->timeout_ctr = 0;
                Serial.println("Error: Expected end transfer");
                huplink->prev_state = UPLINK_COMPLETE;
                huplink->state = UPLINK_ERROR;    
            }
            else
            {
                huplink->timeout_ctr++;
            }
            break;
        }
        case UPLINK_ERROR:
        {
            Serial.println("Uplink ERROR");
            huplink->prev_state = UPLINK_ERROR;
            huplink->state = UPLINK_IDLE;
            hcomms.state = COMMS_IDLE;
            break;
        }
        default:
        {
            huplink->state = UPLINK_IDLE;
            hcomms.state = COMMS_IDLE;
            break;
        }
    }

}



bool COMMS_receiveFileInfo(FILE_Handler_t* hfile, COMMS_uplinkHandler_t* huplink)
{
    UART_msg_t msg;
    if (!UART_receive(&Serial3, &msg, DEFAULT_UART_TIMEOUT_US))
    {
        Serial.println("WARNING: No UART Received while waiting for file info");
        return false;
    }
    if (msg.id != UPLINK_FILE_INFO_ID)
    {
        Serial.println("WARNING: Incorrect File info ID: " + String(msg.id));
        return false;
    }
    if (msg.length <  FILE_INFO_BYTES)
    {
        Serial.println("WARNING: File info length too short");
        return false;
    }
    if (msg.payload[0] != hfile->metadata.ID)
    {
        Serial.println("WARNING: File info ID does not match metadata");
        return false;
    }
    memcpy(&hfile->metadata.chunk_size, &msg.payload[1], sizeof(uint32_t));
    memcpy(&huplink->file_chunks, &msg.payload[5], sizeof(uint32_t));
    return true;
}

void COMMS_sendFileInfo(uint8_t fileID, uint32_t chunk_size, uint32_t num_chunks)
{
    UART_msg_t msg;
    uint8_t data[WOD_INFO_BYTES];

    msg.sof    = UART_SOF;
    msg.id     = WOD_INFO_ID;
    msg.length = WOD_INFO_BYTES;

    data[0] = fileID;
    Serial.println(chunk_size);

    memcpy(&data[1], &chunk_size, sizeof(uint32_t));
    memcpy(&data[5], &num_chunks, sizeof(uint32_t));

    memcpy(msg.payload, data, WOD_INFO_BYTES);

    UART_transmit(&Serial3, &msg);

    Serial.println("Sent WOD INFO");
}


bool COMMS_sendPacket(uint8_t id, const uint8_t *payload, uint8_t length)
{
    UART_msg_t msg = {0};
    msg.sof    = UART_SOF;
    msg.id     = id;
    msg.length = length;
    if (length > 0 && payload != NULL)
    {
        memcpy(msg.payload, payload, length);
    }
    UART_transmit(&Serial3, &msg);
    return true;
}

bool COMMS_receivePacket(Packet_t* packet)
{
    UART_msg_t msg = {0};
    if (packet == NULL)
    {
        Serial.println("Warning: NULL packet");
        return false;
    }
    if (!UART_receive(&Serial3, &msg, 20000))
    {
        Serial.println("Warning: No packet received");
        return false;
    }
    if (msg.id != CHUNK_ID)
    {
        Serial.println("Warning: Message ID not chunk ID. Received" + String(msg.id));
        return false;
    }
    if (msg.length < 3)
    {
        Serial.println("Warning: packet too small");
        return false;
    }
    uint8_t packet_len = msg.length;
    if (packet_len > MAX_PACKET_SIZE)
    {
        Serial.println("Warning: Packet exceeds maximum packet size");
        return false;
    }
    //First two bytes of the packet are the packet index
    packet->packet_idx = ((uint16_t)msg.payload[0] << 8) | msg.payload[1];  
    packet->length = packet_len; //Note that the length includes the two index bytes
    memcpy(packet->payload, &msg.payload[2], packet->length);
    return true;
}