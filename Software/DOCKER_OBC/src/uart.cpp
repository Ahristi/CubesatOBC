#include "uart.h"


bool UART_receive(Stream *port, UART_msg_t* msg, uint32_t timeout_us)
{
    UART_rx_state_t state = UART_RX_WAIT_SOF;
    uint8_t byte;
    uint8_t idx = 0;
    uint8_t crc_idx = 0;

    uint32_t last_byte_time = micros();

    while ((uint32_t)(micros() -  last_byte_time) < timeout_us)
    {
        if (!port->available())
        {
            continue;   // no delay, just keep polling until timeout
        }

        byte = port->read();
        // Reset inter-byte timeout whenever progress is made
        last_byte_time = micros();
        Serial.println("Byte received");
        switch (state)
        {
            case UART_RX_WAIT_SOF:
            {
                if (byte == UART_SOF)
                {
                    memset(msg, 0, sizeof(UART_msg_t));
                    msg->sof = byte;
                    idx = 0;
                    crc_idx = 0;
                    state = UART_RX_GET_ID;
                }
                break;
            }

            case UART_RX_GET_ID:
            {
                msg->id = byte;
                state = UART_RX_GET_LENGTH;
                break;
            }

            case UART_RX_GET_LENGTH:
            {
                msg->length = byte;

                if (msg->length == 0 || msg->length > RX_BUFFER_BYTES)
                {
                    Serial.println("Warning: UART message too big");
                    return false;
                }
                Serial.println("Length: " + String(msg->length));

                state = UART_RX_READ_PAYLOAD;
                break;
            }

            case UART_RX_READ_PAYLOAD:
            {
                msg->payload[idx++] = byte;
                if (idx >= msg->length)
                {
                    state = UART_RX_READ_CRC;
                }
                break;
            }

            case UART_RX_READ_CRC:
            {
                msg->crc |= ((uint16_t)byte << (8 * crc_idx));
                crc_idx++;
                if (crc_idx == RX_CRC_BYTES)
                {
                    Serial.println("Checking CRC");
                    return UART_checkCRC(msg);
                }
                break;
            }

            default:
            {
                state = UART_RX_WAIT_SOF;
                break;
            }
        }
    }
    return false;
}

void UART_transmit(Stream *port, UART_msg_t* msg)
{
    uint8_t data[RX_HEADER_BYTES + RX_BUFFER_BYTES + RX_CRC_BYTES];

    if (msg->length > RX_BUFFER_BYTES)
    {
        Serial.println("ERROR: UART Message is too big");
        return;
    }
    msg->sof = UART_SOF;
    data[0] = msg->sof;
    data[1] = msg->id;
    data[2] = msg->length;
    memcpy(&data[RX_HEADER_BYTES], msg->payload, msg->length);
    msg->crc = UART_crc16_ccitt(data, msg->length + RX_HEADER_BYTES);
    uint16_t crc_offset = RX_HEADER_BYTES + msg->length;
    data[crc_offset]     = msg->crc & 0xFF;
    data[crc_offset + 1] = msg->crc >> 8;

    port->write(data, msg->length + RX_HEADER_BYTES + RX_CRC_BYTES);

}



bool UART_checkCRC(UART_msg_t* msg)
{
    uint16_t crc;
    uint8_t data[RX_HEADER_BYTES + RX_BUFFER_BYTES];
    data[0] = msg->sof;
    data[1] = msg->id;
    data[2] = msg->length;
    memcpy(&data[3], msg->payload, msg->length);
    crc = UART_crc16_ccitt(data, msg->length + RX_HEADER_BYTES);
    return (crc == msg->crc);
}

uint16_t UART_crc16_ccitt(const uint8_t *data, uint16_t length)
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

