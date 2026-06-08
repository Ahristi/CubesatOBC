#include "packet.h"




bool PACKET_send(Packet_t* packet, HardwareSerialIMXRT* port)
{
    UART_msg_t msg = {0};
    msg.sof    = UART_SOF;
    msg.id     = packet->id;
    msg.length = packet->length + PACKET_INDEX_BYTES;
    uint8_t uart_payload[192] = {0};
    uart_payload[0] = (packet->packet_idx >> 8);
    uart_payload[1] = (packet->packet_idx & 0xFF);
    if (packet->length > 0 && packet->payload != NULL)
    {
        memcpy(&msg.payload[2], packet->payload, packet->length);
    }
    else
    {
        return false;
    }
    UART_transmit(port, &msg);
    return true;
}

bool PACKET_receive(Packet_t* packet, HardwareSerialIMXRT* port)
{
    if (packet == nullptr || port == nullptr)
    {
        return false;
    }
    UART_msg_t msg = {0};
    if (!UART_receive(port, &msg, DEFAULT_UART_TIMEOUT_US))
    {
        return false;
    }
    if (msg.length < PACKET_INDEX_BYTES)
    {
        Serial.println("Bad packet length");
        return false;
    }
    packet->id = msg.id;
    packet->packet_idx =((uint16_t)msg.payload[0] << 8) |  ((uint16_t)msg.payload[1]);
    packet->length = msg.length - PACKET_INDEX_BYTES;
    if (packet->length > 0)
    {
        if (packet->payload == nullptr)
        {
            Serial.println("Packet payload buffer is null");
            return false;
        }
        memcpy(packet->payload, &msg.payload[PACKET_INDEX_BYTES], packet->length);
    }
    return true;
}