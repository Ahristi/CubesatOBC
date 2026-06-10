#include "packet.h"

/**
 * @brief Copy packet struct information into a UART message format.
 *        Basically just an extra layer on top of the UART stack that
 *        adds the index of the packet for file transfer.
 *
 * @param packet Pointer to a packet to be sent on UART.
 * @param port   Pointer to the Arduino HAL UART port to send the packet.
 *
 * @retval True if packet sent successfully.
 */
bool PACKET_send(Packet_t* packet, HardwareSerialIMXRT* port)
{
    if (packet == nullptr || port == nullptr)
    {
        Serial.println("ERROR: Null Packet");
        return false;
    }

    if (packet->length == 0 || packet->payload == nullptr)
    {
        Serial.println("ERROR: No payload");
        return false;
    }

    UART_msg_t msg = {0};

    msg.sof    = UART_SOF;
    msg.id     = packet->id;
    msg.length = packet->length + PACKET_INDEX_BYTES;

    memcpy(&msg.payload[0], &packet->packet_idx, PACKET_INDEX_BYTES);
    memcpy(&msg.payload[PACKET_INDEX_BYTES], packet->payload, packet->length);
    Serial.println("Transmitting UART");
    UART_transmit(port, &msg);

    return true;
}

/**
 * @brief Poll the UART message and wait for a packet to be received.
 *
 * @param packet Pointer to a packet to be received.
 *               The packet ID should already be set for filtering.
 * @param port   Pointer to the Arduino HAL UART port to receive the packet.
 *
 * @retval True if a packet was received.
 *
 * @note This function will discard any other UART messages that are received.
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
        Serial.println("WARNING: No Packet Received");
        return false;
    }

    if (msg.id != packet->id)
    {
        Serial.println("WARNING: Packet ID Incorrect. Received " + String(msg.id));
        return false;
    }

    if (msg.length < PACKET_INDEX_BYTES)
    {
        Serial.println("Bad packet length. Got " + String(msg.length));
        return false;
    }

    memcpy(&packet->packet_idx, &msg.payload[0], PACKET_INDEX_BYTES);

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