#include "adcs.h"
#include <Arduino.h>
 
ADCS_Handler_t hadcs;

void ADCS_Init(void)
{
    hadcs.detumble_scale = DETUMBLE_SCALE_START;
    Serial1.begin(ADCS_BAUDRATE);
    Serial.println("ADCS UART initialised on Serial4");
}

void ADCS_task(void)
{
    ADCS_getTelemetry();
    return;
}

void ADCS_getTelemetry(void)
{
    ADCS_rx_state_t state = ADCS_RX_WAIT_SOF;
    uint8_t byte;
    uint8_t idx;
    uint8_t packet_len;
    uint8_t id;
    uint8_t packet[ADCS_RX_BUFFER_LEN];

    while (Serial1.available())
    {
        
        byte = Serial1.read();
        switch (state)
        {
            case ADCS_RX_WAIT_SOF:
            {
                if (byte == ADCS_SOF)
                {
                    Serial.println("ADCS SOF received");
                    packet[0] = byte;
                    idx = 1;
                    state = ADCS_RX_GET_ID;
                }
                break;
            }
            case ADCS_RX_GET_ID:
            {
                id = byte;
                packet[1] = id;
                idx = 2;
                packet_len = 2 + ADCS_getRxPayloadLength(id) + ADCS_RX_CRC_BYTES; //SOF+ID+PAYLOAD+CRC
                if (packet_len == 0 || packet_len > ADCS_RX_BUFFER_LEN)
                {
                    state = ADCS_RX_WAIT_SOF;
                    break;
                }
                state = ADCS_RX_READ_PAYLOAD;
                break;
            }
            case ADCS_RX_READ_PAYLOAD:
            {
                packet[idx++] = byte;
                if (idx >= packet_len)
                {
                    state = ADCS_RX_WAIT_SOF;
                    ADCS_processPacket(id, packet, idx);
                }
                break;
            }
            default:
            {
                state = ADCS_RX_WAIT_SOF;
                break;
            }
        }
    }
}

void ADCS_processPacket(uint8_t id, uint8_t *packet, uint8_t packet_len)
{
    uint8_t payload_len = ADCS_getRxPayloadLength(id);
    uint8_t expected_len = 2 + payload_len + ADCS_RX_CRC_BYTES;

    if (packet_len != expected_len)
    {
        Serial.print("Bad ADCS packet length. RX=");
        Serial.print(packet_len);
        Serial.print(" Expected=");
        Serial.println(expected_len);
        return;
    }

    uint16_t calc_crc = crc16_ccitt(packet, 2 + payload_len);
    uint16_t rx_crc =
        ((uint16_t)packet[2 + payload_len]) |
        ((uint16_t)packet[2 + payload_len + 1] << 8);

    if (rx_crc != calc_crc)
    {
        Serial.print("CRC Failed. RX=0x");
        Serial.print(rx_crc, HEX);
        Serial.print(" CALC=0x");
        Serial.println(calc_crc, HEX);
        return;
    }

    Serial.println("CRC Passed!");

    if (id == ADCS_PACKET_TELEMETRY)
    {
        uint8_t *payload = &packet[2];

        float *f = (float *)payload;

        hadcs.roll      = f[0];
        hadcs.pitch     = f[1];
        hadcs.yaw       = f[2];

        hadcs.roll_dot  = f[3];
        hadcs.pitch_dot = f[4];
        hadcs.yaw_dot   = f[5];

        hadcs.rw1       = f[6];
        hadcs.rw2       = f[7];
        hadcs.rw3       = f[8];

        hadcs.it1       = f[9];
        hadcs.it2       = f[10];
        hadcs.it3       = f[11];

        hadcs.detumble_scale = f[12];

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

uint16_t crc16_ccitt(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFF;

    for (uint16_t i = 0; i < length; i++)
    {
        crc ^= ((uint16_t)data[i] << 8);

        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x8000)
            {
                crc = (crc << 1) ^ 0x1021;
            }
            else
            {
                crc <<= 1;
            }
        }
    }
    return crc;
}
