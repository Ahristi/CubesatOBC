#include "uart.h"


bool UART_receive(Stream *port, UART_msg_t* msg)
{
    // Internal persistent parser state.
    // Keeps the public function prototype unchanged.
    struct UART_RxContext {
        Stream *port = nullptr;
        UART_rx_state_t state = UART_RX_WAIT_SOF;
        uint8_t idx = 0;
        uint8_t crc_idx = 0;
        UART_msg_t working_msg;
    };

    // Increase if you later parse more than four UART/Stream objects.
    static UART_RxContext ctxs[4];

    auto getContext = [&](Stream *p) -> UART_RxContext* {
        for (int i = 0; i < 4; i++) {
            if (ctxs[i].port == p) {
                return &ctxs[i];
            }
        }

        for (int i = 0; i < 4; i++) {
            if (ctxs[i].port == nullptr) {
                ctxs[i].port = p;
                ctxs[i].state = UART_RX_WAIT_SOF;
                ctxs[i].idx = 0;
                ctxs[i].crc_idx = 0;
                memset(&ctxs[i].working_msg, 0, sizeof(UART_msg_t));
                return &ctxs[i];
            }
        }

        return nullptr;
    };

    UART_RxContext *ctx = getContext(port);
    if (ctx == nullptr) {
        return false;
    }

    auto resetContext = [&]() {
        ctx->state = UART_RX_WAIT_SOF;
        ctx->idx = 0;
        ctx->crc_idx = 0;
        memset(&ctx->working_msg, 0, sizeof(UART_msg_t));
    };

    while (port->available())
    {
        uint8_t byte = (uint8_t)port->read();

        switch (ctx->state)
        {
            case UART_RX_WAIT_SOF:
            {
                if (byte == UART_SOF)
                {
                    resetContext();
                    ctx->working_msg.sof = byte;
                    ctx->state = UART_RX_GET_ID;
                }
                break;
            }

            case UART_RX_GET_ID:
            {
                ctx->working_msg.id = byte;
                ctx->state = UART_RX_GET_LENGTH;
                break;
            }

            case UART_RX_GET_LENGTH:
            {
                ctx->working_msg.length = byte;

                if (ctx->working_msg.length == 0 ||
                    ctx->working_msg.length > RX_BUFFER_BYTES)
                {
                    resetContext();
                    break;
                }

                ctx->state = UART_RX_READ_PAYLOAD;
                break;
            }

            case UART_RX_READ_PAYLOAD:
            {
                ctx->working_msg.payload[ctx->idx++] = byte;

                if (ctx->idx >= ctx->working_msg.length)
                {
                    ctx->state = UART_RX_READ_CRC;
                }

                break;
            }

            case UART_RX_READ_CRC:
            {
                ctx->working_msg.crc |= ((uint16_t)byte << (8 * ctx->crc_idx));
                ctx->crc_idx++;

                if (ctx->crc_idx >= RX_CRC_BYTES)
                {
                    bool crc_ok = UART_checkCRC(&ctx->working_msg);

                    if (crc_ok)
                    {
                        memcpy(msg, &ctx->working_msg, sizeof(UART_msg_t));
                    }

                    resetContext();
                    return crc_ok;
                }

                break;
            }

            default:
            {
                resetContext();
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
        return;
    }
    msg->sof = UART_SOF;
    data[0] = msg->sof;
    data[1] = msg->id;
    data[2] = msg->length;
    memcpy(&data[RX_HEADER_BYTES], msg->payload, msg->length);
    msg->crc = UART_crc16_ccitt(data, msg->length + RX_HEADER_BYTES);
    uint8_t crc_offset = RX_HEADER_BYTES + msg->length;
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

