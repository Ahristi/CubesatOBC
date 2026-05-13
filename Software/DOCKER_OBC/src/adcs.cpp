#include "adcs.h"
#include <Arduino.h>
 
ADCS_Handler_t hadcs;

void ADCS_Init(void)
{
    hadcs.detumble_scale = DETUMBLE_SCALE_START;
    hadcs.attitude_command_ready   = false;
    hadcs.orbital_parameters_ready = false;
    hadcs.detumble_command_ready   = false;
    hadcs.pointing_command_ready   = false;
    Serial1.begin(ADCS_BAUDRATE);
    Serial.println("ADCS UART initialised on Serial4");
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
