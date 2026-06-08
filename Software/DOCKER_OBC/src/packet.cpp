#include "packet.h"



/**
 * @brief Copy packet struct information into a UART message format.
 *        Basically just an extra layer on top of the UART stack that 
 *        Adds the index of the packet for file transfer.
 * 
 * @param packet    pointer to a packet to be sent on UART. Assumes the packet
 *                  has already been initialised before the function call 
 * 
 * @param port      pointer to the Arduino HAL UART port to send the packet.
 * 
 * @retval True if packet sent successfully. 
 * 
*/
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
/**
 * @brief Poll the UART message and wait for a packet to be received.
 * 
 * @param packet    pointer to a packet to be sent on UART. Assumes the packet
 *                  has already been initialised and the ID has been set correctly for filtering.
 * 
 * @param port      pointer to the Arduino HAL UART port to receive the packet.
 * 
 * @retval True if a packet was received.
 * 
 * @note This function will discard any other UART messages that are received. Should be fine since the 
 *       scheduler is deterministic but still use carefully.
 * 
*/
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
    if (msg.id != packet->id)
    {
        //Filter for the packet IDs we want. Use carefully as this will completely discard any UART messages we receive.
        return false;
    }
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