#include "uart.h"


bool UART_getMessage(Stream *port, UART_msg_t* msg)
{
    UART_rx_state_t state = UART_RX_WAIT_SOF;
    uint8_t byte;
    uint8_t idx = 0;
    uint8_t crc_idx = 0;
    uint16_t crc;

    while (port->available())
    {   
        byte = port->read();
        switch (state)
        {
            case UART_RX_WAIT_SOF:
            {
                if (byte == UART_SOF)
                {
                    Serial.println("UART SOF received");
                    msg->sof = byte;
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
                    return false;
                }
                Serial.println("UART length received")
                state = UART_RX_READ_PAYLOAD;
            }
            case UART_RX_READ_PAYLOAD:
            {
                msg->packet[idx++] = byte;
                if (idx >= msg->length)
                {
                    state = UART_RX_WAIT_SOF;
                }
                break;
            }
            case UART_RX_READ_CRC:
            {
                msg->crc |= (byte << crc_idx);
                crc_idx++;
                if (crc_idx = RX_CRC_BYTES)
                {
                    state = UART_RX_CHECK_CRC;
                }
                break;
            }
            case UART_RX_CHECK_CRC:
            {
                return  UART_checkCRC(msg);
            }
            default:
            {
                state = ADCS_RX_WAIT_SOF;
                break;
            }
        }
    }
}

bool UART_checkCRC(UART_msg_t* msg)
{
    uint16_t crc;
    uint8_t data[RX_BUFFER_BYTES]
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