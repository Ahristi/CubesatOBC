#include "payload.h"
#include <Arduino.h>


static bool PAYLOAD_sendChunk(FILE_Handler_t* hfile);

PAYLOAD_Handler_t hpayload;
uint8_t payload_serial_buffer[PAYLOAD_UART_BUFFER_SIZE];




static FILE_Handler_t experiment_file = {
    .metadata_file_name = EXPERIMENT_META_FILE_NAME,
    .file_name          = EXPERIMENT_DATA_FILE_NAME,
    .metadata = {
        .ID         = EXPERIMENT_META_ID,
        .chunk_size = 0,
        .num_chunks = 0,
        .max_chunks = EXPERIMENT_MAX_RECORDS,
        .read_ptr   = 0
    }
};

static FILE_Handler_t results_file = {
    .metadata_file_name = RESULT_META_FILE_NAME,
    .file_name          = RESULT_DATA_FILE_NAME,
    .metadata = {
        .ID         = RESULT_META_ID,
        .chunk_size = 0,
        .num_chunks = 0,
        .max_chunks = RESULT_MAX_RECORDS,
        .read_ptr   = 0
    }
};

void PAYLOAD_Init()
{
    hpayload.prev_state          = PAYLOAD_OFF;
    hpayload.state               = PAYLOAD_OFF;
    hpayload.experiment_finished = true;
    hpayload.experiment_ready    = false;
    hpayload.start_experiment    = false;
    hpayload.num_result_chunks   = 0;
    hpayload.timeout_ctr         = 0;

    hpayload.experiment_file = experiment_file;
    hpayload.results_file    = results_file;
    FILE_initialiseMetadata(&hpayload.experiment_file);
    Serial.println("Experiment File Chunks: " + String(hpayload.experiment_file.metadata.num_chunks));
    FILE_initialiseMetadata(&hpayload.results_file);
    Serial.println("Result File Chunks: " + String(hpayload.results_file.metadata.num_chunks));
    hpayload.serial = &Serial2;
    hpayload.serial->addMemoryForRead(payload_serial_buffer, PAYLOAD_UART_BUFFER_SIZE);
    hpayload.serial->begin(PAYLOAD_BAUD_RATE);
}

void PAYLOAD_task()
{
    switch (hpayload.state)
    {
        case PAYLOAD_OFF:
        {
            if (hpayload.start_experiment)
            {
                hpayload.experiment_finished = false;
                hpayload.timeout_ctr = 0;
                PAYLOAD_setState(PAYLOAD_BOOT);
            }
            break;
        }

        case PAYLOAD_BOOT:
        {
            if (PAYLOAD_getOnMSG())
            {
                Serial.println("Payload is on. Sending ack then experiment file info.");
                hpayload.timeout_ctr = 0;
                PAYLOAD_setState(PAYLOAD_SEND_ACK);
            }
            /*
            else if (hpayload.timeout_ctr >= PAYLOAD_START_TIMEOUT_COUNT)
            {
                Serial.println("ERROR: Timed out while waiting for payload start.");
                hpayload.timeout_ctr = 0;
                PAYLOAD_setState(PAYLOAD_ERROR);
            }
            else
            {
                hpayload.timeout_ctr++;
            }
            */
            break;
        }

        case PAYLOAD_SEND_INFO:
        {
            Serial.println("Sending payload experiment info");
            PAYLOAD_sendFileInfo(&hpayload.experiment_file, hpayload.serial);
            PAYLOAD_setState(PAYLOAD_WAIT_ACK);
            break;
        }

        case PAYLOAD_SEND_EXPERIMENT:
        {          
            if (PAYLOAD_sendChunk(&hpayload.experiment_file))
            {
                Serial.print("Sent experiment chunk ");
                Serial.print(hpayload.experiment_file.metadata.read_ptr + 1);
                Serial.print(" / ");
                Serial.println(hpayload.experiment_file.metadata.num_chunks);

                PAYLOAD_setState(PAYLOAD_WAIT_ACK);
            }
            else
            {
                Serial.println("ERROR: Failed to send experiment chunk");
                PAYLOAD_setState(PAYLOAD_ERROR);
            }
            break;
        }
        case PAYLOAD_START_EXPERIMENT:
        {
            PAYLOAD_start();
            PAYLOAD_setState(PAYLOAD_WAIT_ACK);
            break;
        }

        case PAYLOAD_WAIT_ACK:
        {
            if (PAYLOAD_getAck())
            {
                hpayload.timeout_ctr = 0;
                hpayload.ack_retry_ctr = 0;

                if (hpayload.prev_state == PAYLOAD_SEND_INFO)
                {
                    hpayload.experiment_file.metadata.read_ptr = 0;

                    if (hpayload.experiment_file.metadata.num_chunks == 0)
                    {
                        Serial.println("ERROR: Empty experiment file");
                        PAYLOAD_setState(PAYLOAD_SEND_END_FILE);
                    }
                    else
                    {
                        FILE_open(&hpayload.experiment_file, FILE_OPEN_FOR_READ);
                        PAYLOAD_setState(PAYLOAD_SEND_EXPERIMENT);
                    }
                }
                else if (hpayload.prev_state == PAYLOAD_START_EXPERIMENT)
                {
                    PAYLOAD_setState(PAYLOAD_RUNNING);
                }
                else if (hpayload.prev_state == PAYLOAD_SEND_EXPERIMENT)
                {
                    hpayload.experiment_file.metadata.read_ptr++;

                    if (hpayload.experiment_file.metadata.read_ptr >= hpayload.experiment_file.metadata.num_chunks)
                    {
                        PAYLOAD_setState(PAYLOAD_SEND_END_FILE);
                    }
                    else
                    {
                        PAYLOAD_setState(PAYLOAD_SEND_EXPERIMENT);
                    }
                }
                else if (hpayload.prev_state == PAYLOAD_SEND_END_FILE)
                {
                    Serial.println("Experiment file sent. Waiting for payload results...");
                    hpayload.timeout_ctr = 0;
                    PAYLOAD_setState(PAYLOAD_START_EXPERIMENT);
                }
            }
            else if (hpayload.timeout_ctr >= PAYLOAD_ACK_TIMEOUT_COUNT)
            {
                hpayload.timeout_ctr = 0;
                hpayload.ack_retry_ctr++;

                Serial.print("WARNING: Timed out waiting for payload ACK. Retry ");
                Serial.print(hpayload.ack_retry_ctr);
                Serial.print(" / ");
                Serial.println(PAYLOAD_MAX_ACK_RETRIES);

                if (hpayload.ack_retry_ctr >= PAYLOAD_MAX_ACK_RETRIES)
                {
                    Serial.println("ERROR: Maximum ACK retries exceeded");
                    hpayload.ack_retry_ctr = 0;
                    PAYLOAD_setState(PAYLOAD_ERROR);
                }
                else
                {
                    /*
                    * Resend the message that originally caused us to wait for ACK.
                    *
                    * Important:
                    * Do not increment read_ptr here.
                    * The same chunk should be resent with the same packet_idx.
                    */
                    if (hpayload.prev_state == PAYLOAD_SEND_INFO)
                    {
                        PAYLOAD_setState(PAYLOAD_SEND_INFO);
                    }
                    else if (hpayload.prev_state == PAYLOAD_SEND_EXPERIMENT)
                    {
                        PAYLOAD_setState(PAYLOAD_SEND_EXPERIMENT);
                    }
                    else if (hpayload.prev_state == PAYLOAD_START_EXPERIMENT)
                    {
                        PAYLOAD_setState(PAYLOAD_START_EXPERIMENT);
                    }
                    else if (hpayload.prev_state == PAYLOAD_SEND_END_FILE)
                    {
                        PAYLOAD_setState(PAYLOAD_SEND_END_FILE);
                    }
                    else
                    {
                        Serial.println("ERROR: Cannot retry unknown previous payload state");
                        hpayload.ack_retry_ctr = 0;
                        PAYLOAD_setState(PAYLOAD_ERROR);
                    }
                }
            }
            else
            {
                hpayload.timeout_ctr++;
            }

            break;
        }

        case PAYLOAD_SEND_END_FILE:
        {
            FILE_close(&hpayload.experiment_file);
            PAYLOAD_sendEndTransfer();
            Serial.println("Payload experiment transfer complete");
            FILE_writeMetadata(&hpayload.experiment_file);
            PAYLOAD_setState(PAYLOAD_WAIT_ACK);
            break;
        }

        case PAYLOAD_RUNNING:
        {
            if (PAYLOAD_receiveFileInfo(&hpayload.results_file, hpayload.serial))
            {
                Serial.println("Received payload result file info. Clearing Results file.");
                FILE_clear(&hpayload.results_file);
                hpayload.timeout_ctr = 0;
                hpayload.results_file.metadata.num_chunks = 0;
                hpayload.results_file.metadata.read_ptr = 0;

                if (!FILE_open(&hpayload.results_file, FILE_OPEN_FOR_WRITE))
                {
                    Serial.println("ERROR: Failed to open results file for writing");
                    PAYLOAD_setState(PAYLOAD_ERROR);
                    break;
                }

                PAYLOAD_setState(PAYLOAD_SEND_ACK);
            }
            else if (hpayload.timeout_ctr >= PAYLOAD_EXPERIMENT_TIMEOUT_COUNT)
            {
                Serial.println("ERROR: Timed out waiting for payload to finish experiment");
                hpayload.timeout_ctr = 0;
                PAYLOAD_setState(PAYLOAD_ERROR);
            }
            else
            {
                hpayload.timeout_ctr++;
            }
            break;
        }

        case PAYLOAD_GET_RESULTS:
        {
            if (PAYLOAD_receiveChunk())
            {
                Serial.print("Received result chunk ");
                Serial.print(hpayload.results_file.metadata.read_ptr + 1);
                Serial.print(" / ");
                Serial.println(hpayload.num_result_chunks);

                hpayload.results_file.metadata.read_ptr++;
                hpayload.timeout_ctr = 0;
                PAYLOAD_setState(PAYLOAD_SEND_ACK);
            }
            else if (hpayload.timeout_ctr >= PAYLOAD_ACK_TIMEOUT_COUNT)
            {
                Serial.println("ERROR: Timed out waiting for result chunk");
                hpayload.timeout_ctr = 0;
                PAYLOAD_setState(PAYLOAD_ERROR);
            }
            else
            {
                hpayload.timeout_ctr++;
            }
            break;
        }

        case PAYLOAD_SEND_ACK:
        {
            PAYLOAD_sendAck();
            Serial.println("Sending ACK");

            if (hpayload.prev_state == PAYLOAD_BOOT)
            {
                PAYLOAD_setState(PAYLOAD_SEND_INFO);
            }
            else if (hpayload.prev_state == PAYLOAD_RUNNING)
            {
                if (hpayload.num_result_chunks== 0)
                {
                    PAYLOAD_setState(PAYLOAD_END);
                }
                else
                {
                    PAYLOAD_setState(PAYLOAD_GET_RESULTS);
                }
            }
            else if (hpayload.prev_state == PAYLOAD_GET_RESULTS)
            {
                if (hpayload.results_file.metadata.read_ptr >= hpayload.num_result_chunks)
                {
                    PAYLOAD_setState(PAYLOAD_END);
                }
                else
                {
                    PAYLOAD_setState(PAYLOAD_GET_RESULTS);
                }
            }
            else if (hpayload.prev_state == PAYLOAD_END)
            {
                Serial.println("Payload result transfer complete");

                FILE_writeMetadata(&hpayload.results_file);

                hpayload.experiment_finished = true;
                hpayload.start_experiment    = false;
                hpayload.experiment_ready    = false;
                hpayload.timeout_ctr         = 0;

                PAYLOAD_setState(PAYLOAD_OFF);
            }
            break;
        }

        case PAYLOAD_END:
        {
            if (PAYLOAD_getEndTransfer())
            {
                Serial.println("Received payload end transfer");
                hpayload.timeout_ctr = 0;
                PAYLOAD_setState(PAYLOAD_SEND_ACK);
            }
            else if (hpayload.timeout_ctr >= PAYLOAD_ACK_TIMEOUT_COUNT)
            {
                Serial.println("ERROR: Timed out waiting for payload end transfer");
                hpayload.timeout_ctr = 0;
                PAYLOAD_setState(PAYLOAD_ERROR);
            }
            else
            {
                hpayload.timeout_ctr++;
            }
            break;
        }

        case PAYLOAD_ERROR:
        {
            FILE_close(&hpayload.experiment_file);
            hpayload.experiment_ready    = false;
            hpayload.experiment_finished = true;
            hpayload.start_experiment    = false;
            hpayload.timeout_ctr         = 0;
            PAYLOAD_setState(PAYLOAD_OFF);
            break;
        }

        default:
        {
            PAYLOAD_setState(PAYLOAD_ERROR);
            break;
        }
    }
}

bool PAYLOAD_getOnMSG(void)
{
    UART_msg_t msg = {0};

    if (!UART_receive(hpayload.serial, &msg, DEFAULT_UART_TIMEOUT_US))
    {
        return false;
    }

    if (msg.id != PAYLOAD_ON_ID)
    {
        Serial.println("Warning: Bad command received from payload board.");
        return false;
    }

    if (msg.length < 1)
    {
        Serial.println("Bad start message length");
        return false;
    }

    if (msg.payload[0] != PAYLOAD_ON_ID)
    {
        Serial.println("Bad start message payload");
        return false;
    }

    return true;
}

/**
 * @brief   Reads the experiment file at the current read pointer and sends the packet.
 *
 * @retval  True if chunk was sent successfully.
 */
static bool PAYLOAD_sendChunk(FILE_Handler_t* hfile)
{
    if (hfile == nullptr)
    {
        Serial.println("ERROR: Null file handler in PAYLOAD_sendChunk");
        return false;
    }

    if (hfile->metadata.chunk_size > MAX_CHUNK_SIZE)
    {
        Serial.println("ERROR: Chunk size exceeds MAX_CHUNK_SIZE");
        return false;
    }

    if (hfile->metadata.read_ptr > UINT16_MAX)
    {
        Serial.println("ERROR: Packet index exceeds uint16_t range");
        return false;
    }

    uint8_t chunk[MAX_CHUNK_SIZE] = {0};
    const size_t bytes_read = FILE_read(hfile, chunk);

    if (bytes_read == 0)
    {
        Serial.println("ERROR: No bytes read from experiment file");
        return false;
    }

    if (bytes_read > MAX_CHUNK_SIZE || bytes_read > UINT8_MAX)
    {
        Serial.println("ERROR: Bytes read exceeds packet payload size");
        return false;
    }

    Packet_t packet = {0};
    packet.id         = EXPERIMENT_CHUNK_ID;
    packet.packet_idx = (uint16_t)hfile->metadata.read_ptr;
    packet.length     = (uint8_t)bytes_read;
    Serial.println("SENDING PACKET WITH:");
    Serial.println("ID: " + String(packet.id));
    Serial.println("Idx: " + String(packet.packet_idx));
    Serial.println("Len: " + String(packet.length));
    memcpy(packet.payload, chunk, bytes_read);

    return PACKET_send(&packet, hpayload.serial);
}



/**
 * @brief   Waits to receive chunk packets from the payload and writes them to the results file.
 *
 * @retval  True if chunk was received and written to the SD card.
 */
bool PAYLOAD_receiveChunk()
{
    Packet_t packet = {0};
    packet.id = 0x69;

    if (!PACKET_receive(&packet, hpayload.serial))
    {
        return false;
    }

    if (packet.id != EXPERIMENT_CHUNK_ID)
    {
        Serial.println("WARNING: Incorrect payload result chunk ID");
        return false;
    }

    if (packet.packet_idx != hpayload.results_file.metadata.read_ptr)
    {
        Serial.print("WARNING: Unexpected result packet index. Expected ");
        Serial.print(hpayload.results_file.metadata.read_ptr);
        Serial.print(", got ");
        Serial.println(packet.packet_idx);
        return false;
    }

    if (!FILE_write(&hpayload.results_file, packet.payload, packet.length))
    {
        Serial.println("ERROR: Failed to write result chunk to SD");
        return false;
    }

    return true;
}

/**
 * @brief   Sends an ACK message on the payload UART port.
 *
 * @retval  True if ack was sent successfully.
 */
bool PAYLOAD_sendAck(void)
{
    UART_msg_t msg = {0};

    msg.sof        = UART_SOF;
    msg.id         = PAYLOAD_ACK_ID;
    msg.length     = 1;
    msg.payload[0] = PAYLOAD_ACK_ID;

    UART_transmit(hpayload.serial, &msg);

    return true;
}

bool PAYLOAD_getAck(void)
{
    UART_msg_t msg = {0};

    if (!UART_receive(hpayload.serial, &msg, DEFAULT_UART_TIMEOUT_US))
    {
        return false;
    }

    if (msg.id != PAYLOAD_ACK_ID)
    {
        Serial.println("Warning: Bad command received from payload board.");
        return false;
    }

    if (msg.length < 1)
    {
        Serial.println("Bad ack message length");
        return false;
    }

    if (msg.payload[0] != PAYLOAD_ACK_ID)
    {
        Serial.println("Bad ack message payload");
        return false;
    }

    return true;
}

void PAYLOAD_setState(PAYLOAD_State_t new_state)
{
    if (hpayload.state != new_state)
    {
        hpayload.prev_state = hpayload.state;
        hpayload.state      = new_state;
    }
}

bool PAYLOAD_sendEndTransfer(void)
{
    UART_msg_t msg = {0};

    msg.sof        = UART_SOF;
    msg.id         = PAYLOAD_END_TRANSFER_ID;
    msg.length     = 1;
    msg.payload[0] = PAYLOAD_END_TRANSFER_ID;

    UART_transmit(hpayload.serial, &msg);
    return true;
}

bool PAYLOAD_getEndTransfer(void)
{
    UART_msg_t msg = {0};

    if (!UART_receive(hpayload.serial, &msg, DEFAULT_UART_TIMEOUT_US))
    {
        return false;
    }

    if (msg.id != PAYLOAD_END_TRANSFER_ID)
    {
        Serial.println("WARNING: Incorrect end transfer message ID");
        return false;
    }

    if (msg.length < 1)
    {
        Serial.println("WARNING: Bad end transfer message length");
        return false;
    }

    if (msg.payload[0] != PAYLOAD_END_TRANSFER_ID)
    {
        Serial.println("WARNING: Bad end transfer payload");
        return false;
    }

    return true;
}

bool PAYLOAD_sendFileInfo(FILE_Handler_t* hfile, HardwareSerialIMXRT* port)
{
    UART_msg_t msg = {0};
    uint8_t data[PAYLOAD_FILE_INFO_BYTES] = {0};

    msg.sof    = UART_SOF;
    msg.id     = PAYLOAD_FILE_INFO_ID;
    msg.length = PAYLOAD_FILE_INFO_BYTES;

    data[0] = hfile->metadata.ID;
    memcpy(&data[1], &hfile->metadata.chunk_size, sizeof(uint32_t));
    memcpy(&data[5], &hfile->metadata.num_chunks, sizeof(uint32_t));

    memcpy(msg.payload, data, PAYLOAD_FILE_INFO_BYTES);
    UART_transmit(port, &msg);

    return true;
}

bool PAYLOAD_receiveFileInfo(FILE_Handler_t* hfile, HardwareSerialIMXRT* port)
{
    UART_msg_t msg = {0};

    if (!UART_receive(port, &msg, DEFAULT_UART_TIMEOUT_US))
    {
        return false;
    }

    if (msg.id != PAYLOAD_FILE_INFO_ID)
    {
        Serial.println("WARNING: Incorrect file info ID: " + String(msg.id));
        return false;
    }

    if (msg.length < PAYLOAD_FILE_INFO_BYTES)
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
    memcpy(&hfile->metadata.num_chunks, &msg.payload[5], sizeof(uint32_t));
    hpayload.num_result_chunks = hfile->metadata.num_chunks;
    return true;
}
void PAYLOAD_start(void)
{
    UART_msg_t msg = {0};
    msg.sof    = UART_SOF;
    msg.id     = PAYLOAD_START_CMD_ID;
    msg.length = 1;
    msg.payload[0] = PAYLOAD_START_CMD_ID;
    UART_transmit(hpayload.serial, &msg);
}