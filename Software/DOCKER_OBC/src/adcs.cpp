#include "adcs.h"
#include <Arduino.h>
 
ADCS_Handler_t hadcs;
ADCS_HardwareHandler_t hwHandle;
ADCS_StateHandle_t stateHandle;
ADCS_hardwareInstruction_t controlHandle;

void ADCS_Init(void)
{
    hadcs.detumble_scale = DETUMBLE_SCALE_START;
    hadcs.attitude_command_ready   = false;
    hadcs.orbital_parameters_ready = false;
    hadcs.detumble_command_ready   = false;
    hadcs.pointing_command_ready   = false;
    Serial1.begin(ADCS_BAUDRATE);
    Serial.println("ADCS UART initialised on Serial1");
}

void ADCS_task(void)
{
    ADCS_getTelemetry();
    ADCS_updateAttitude();
    ADCS_updateOrbitalParameters();
    return;
}

void ADCS_getTelemetry(void)
{
    UART_msg_t msg;
    if (UART_receive(&Serial1, &msg))
    {
        Serial.println("ADCS Message Received!");
        ADCS_processPacket(msg.id, msg.payload, msg.length);
    }
}

void ADCS_getStateControllerUART(Stream* port)
{
    UART_msg_t msg;
    if (UART_receive(port, &msg))
    {
        Serial.println("ADCS Controller Message Received!");
        ADCS_processStateControllerUART(port, msg.id, msg.payload, msg.length);
    }
}


void ADCS_processStateControllerUART(Stream* port, uint8_t id,uint8_t* packet, uint8_t packet_len)
{
    uint8_t expected_length = ADCS_getRxPayloadLength(id);
    if (packet_len != expected_length)
    {
        Serial.print("Bad ADCS packet length. RX=");
        Serial.print(packet_len);
        Serial.print(" Expected=");
        Serial.println(expected_length);
        return;
    }
    if (id == ADCS_PACKET_REQUEST_ID)
    {
        // Respond with hardware info
    }
    if (id == ADCS_PACKET_CONTROL_ID)
    {
        // Respond by loading into memory
    }
}

void ADCS_hardwareRequestResponse(Stream* port)
{
    UART_msg_t msg;
    msg.sof    = UART_SOF;
    msg.id     = ADCS_PACKET_HW_DATA_ID;
    msg.length = sizeof(ADCS_hardwareData_t);
    memcpy(msg.payload, &hwHandle.hw_data, sizeof(ADCS_hardwareData_t));
    UART_transmit(port, &msg);
    Serial.println("Sent hardware data");
}

void ADCS_sendHardwareRequest(Stream* port)
{
    ADCS_requestHardwareData_t hwRequest;
    hwRequest.request = true;

    UART_msg_t msg;
    msg.sof    = UART_SOF;
    msg.id     = ADCS_PACKET_REQUEST_ID;
    msg.length = sizeof(ADCS_requestHardwareData_t);
    memcpy(msg.payload, &hwRequest, sizeof(ADCS_requestHardwareData_t));
    UART_transmit(port, &msg);
    Serial.println("Sent request for hardware data");
}

void ADCS_sendControlInstruction(Stream* port)
{
    UART_msg_t msg;
    msg.sof    = UART_SOF;
    msg.id     = ADCS_PACKET_CONTROL_ID;
    msg.length = sizeof(ADCS_hardwareInstruction_t);
    memcpy(msg.payload, &controlHandle, sizeof(ADCS_hardwareInstruction_t));
    UART_transmit(port, &msg);
    Serial.println("Sent control instruction");
}

void ADCS_processPacket(uint8_t id, uint8_t *payload, uint8_t payload_length)
{
    uint8_t expected_length = ADCS_getRxPayloadLength(id);
    if (payload_length != expected_length)
    {
        Serial.print("Bad ADCS packet length. RX=");
        Serial.print(payload_length);
        Serial.print(" Expected=");
        Serial.println(expected_length);
        return;
    }
    if (id == ADCS_PACKET_TELEMETRY)
    {
        if (payload_length < 13 * sizeof(float))
        {
            Serial.println("ADCS telemetry packet too short");
            return;
        }

        float *p = (float *)payload;

        hadcs.roll      = p[0];
        hadcs.pitch     = p[1];
        hadcs.yaw       = p[2];

        hadcs.roll_dot  = p[3];
        hadcs.pitch_dot = p[4];
        hadcs.yaw_dot   = p[5];

        hadcs.rw1       = p[6];
        hadcs.rw2       = p[7];
        hadcs.rw3       = p[8];

        hadcs.it1       = p[9];
        hadcs.it2       = p[10];
        hadcs.it3       = p[11];

        hadcs.detumble_scale = p[12];

        Serial.println("ADCS telemetry updated");
    }
    if (id == ADCS_PACKET_REQUEST_ID)
    {
        ADCS_hardwareRequestResponse(HW_TO_STATE_SERIAL);
    }
    if (id == ADCS_PACKET_HW_DATA_ID)
    {
        memcpy(&hwHandle.hw_data, payload, sizeof(ADCS_hardwareData_t));
    }
    if (id == ADCS_PACKET_CONTROL_ID)
    {
        memcpy(&hwHandle.ctrl, payload, sizeof(ADCS_hardwareInstruction_t));
    }
}


void ADCS_receivePacket(Stream *port)
{
    UART_msg_t msg;
    UART_receive(port, &msg);
    uint8_t len = ADCS_getRxPayloadLength(msg.id);
    ADCS_processPacket(msg.id, msg.payload, len);
}

uint8_t ADCS_getRxPayloadLength(uint8_t id) 
{
    switch (id)
    {
        case ADCS_PACKET_TELEMETRY:
            return ADCS_PACKET_TELEMETRY_BYTES;
        case ADCS_PACKET_ACK:
            return ADCS_PACKET_ACK_BYTES;
        case ADCS_PACKET_ERROR:
            return ADCS_PACKET_ERROR_BYTES;
        case ADCS_PACKET_CONTROL_ID:
            return sizeof(ADCS_hardwareInstruction_t);
        case ADCS_PACKET_REQUEST_ID:
            return sizeof(ADCS_requestHardwareData_t);
        case ADCS_PACKET_HW_DATA_ID:
            return sizeof(ADCS_hardwareData_t);

        default:
            return 0;
    } 
}
void ADCS_updateAttitude(void)
{
    if (hadcs.attitude_command_ready)
    {
        UART_msg_t msg;
        hadcs.attitude_command_ready = false;
        msg.sof    = UART_SOF;
        msg.id     = ADCS_ATTITUDE_UPDATE_ID;
        msg.length = sizeof(ADCS_attitudeCommand_t);
        memcpy(msg.payload, &hadcs.attitude_command, sizeof(ADCS_attitudeCommand_t));
        UART_transmit(&Serial1, &msg);
        Serial.println("Sent attitude command");
    }
}
void ADCS_updateOrbitalParameters(void)
{
    if (hadcs.orbital_parameters_ready)
    {
        UART_msg_t msg;
        hadcs.orbital_parameters_ready =  false;
        msg.sof    = UART_SOF;
        msg.id     = ADCS_ORBIT_UPDATE_ID;
        msg.length = ADCS_ORBIT_UPDATE_BYTES;
        memcpy(msg.payload, &hadcs.orbital_parameters, sizeof(ADCS_orbitalParameters_t));
        UART_transmit(&Serial1, &msg);
    }
}


void ADCS_debugPrint(void)
{
    Serial.println("========== ADCS ==========");

    Serial.print("Detumble Scale: ");
    Serial.println(hadcs.detumble_scale, 6);

    Serial.println();

    Serial.println("Attitude:");
    Serial.print("  Roll  : ");
    Serial.println(hadcs.roll, 6);

    Serial.print("  Pitch : ");
    Serial.println(hadcs.pitch, 6);

    Serial.print("  Yaw   : ");
    Serial.println(hadcs.yaw, 6);

    Serial.println();

    Serial.println("Angular Velocity:");
    Serial.print("  Roll Dot  : ");
    Serial.println(hadcs.roll_dot, 6);

    Serial.print("  Pitch Dot : ");
    Serial.println(hadcs.pitch_dot, 6);

    Serial.print("  Yaw Dot   : ");
    Serial.println(hadcs.yaw_dot, 6);

    Serial.println();

    Serial.println("Reaction Wheels:");
    Serial.print("  R/W 1  : ");
    Serial.println(hadcs.rw1, 6);

    Serial.print("  R/W 2  : ");
    Serial.println(hadcs.rw2, 6);

    Serial.print("  R/W 3  : ");
    Serial.println(hadcs.rw3, 6);


    Serial.println("Magnetorquers:");
    Serial.print("  Torquer 1  : ");
    Serial.println(hadcs.it1, 6);

    Serial.print("  Torquer 2  : ");
    Serial.println(hadcs.it1, 6);

    Serial.print("  Torquer 3  : ");
    Serial.println(hadcs.it1, 6);

    Serial.println();

    Serial.println("Commands:");
    Serial.print("  Detumble Command Ready : ");
    Serial.println(hadcs.detumble_command_ready ? "TRUE" : "FALSE");

    Serial.print("  Pointing Command Ready : ");
    Serial.println(hadcs.pointing_command_ready ? "TRUE" : "FALSE");

    Serial.println("==========================");
}
