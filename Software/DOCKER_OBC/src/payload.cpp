#include "payload.h"
#include <Arduino.h>


PAYLOAD_Handler_t hpayload;

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
        .ID         = RESULT_FILE_ID,
        .chunk_size = 0,
        .num_chunks = 0,
        .max_chunks = RESULT_MAX_RECORDS,
        .read_ptr   = 0
    }
};


void PAYLOAD_Init()
{
    hpayload.prev_state = PAYLOAD_OFF;
    hpayload.state  = PAYLOAD_OFF;
    hpayload.experiment_finished = true;
    hpayload.experiment_ready = false;
    hpayload.start_experiment = false;
    hpayload.experiment_file = experiment_file;
    hpayload.results_file = results_file;

    Serial4.begin(PAYLOAD_BAUD_RATE);
    Serial.println("ADCS payload initialised on Serial4");
    hpayload.serial = &Serial4;
}

void PAYLOAD_task()
{
    switch (hpayload.state)
    {
        case PAYLOAD_OFF:
        {
            if (hpayload.start_experiment)
            {
                hpayload.prev_state = PAYLOAD_OFF;
                hpayload.state = PAYLOAD_BOOT;
            }
            break;
        }
        case PAYLOAD_BOOT:
        {
            if (PAYLOAD_getStartCMD())
            {
                hpayload.prev_state = PAYLOAD_BOOT;
                hpayload.state = PAYLOAD_SEND_EXPERIMENT;
            }
            break;
        }
        case PAYLOAD_SEND_EXPERIMENT:
        {
            uint8_t chunk[MAX_CHUNK_SIZE];
            size_t bytes_read = FILE_read(&hpayload.experiment_file, chunk);
            PAYLOAD_sendChunk(EXPERIMENT_CHUNK_ID, chunk, (uint8_t)bytes_read);
            Serial.println("Sent chunk");
            Serial.print("Read Pointer: ");
            Serial.println(hpayload.experiment_file.metadata.read_ptr);
            hpayload.prev_state = PAYLOAD_SEND_EXPERIMENT;
            hpayload.state      = PAYLOAD_WAIT_ACK;
            break;
        }
        case PAYLOAD_WAIT_ACK:
        {
            if (PAYLOAD_getAck())
            {
                if (hpayload.prev_state == PAYLOAD_SEND_EXPERIMENT)
                {
                    if (hpayload.experiment_file.metadata.read_ptr == hpayload.experiment_file.metadata.num_chunks)
                    {       
                        hpayload.prev_state = PAYLOAD_WAIT_ACK;
                        hpayload.state = PAYLOAD_SEND_END_FILE;
                    }
                    else
                    {
                        hpayload.prev_state = PAYLOAD_WAIT_ACK;
                        hpayload.state = PAYLOAD_SEND_EXPERIMENT;
                    }
                }
                else if (hpayload.prev_state == PAYLOAD_SEND_END_FILE)
                {
                        hpayload.prev_state = PAYLOAD_WAIT_ACK;
                        hpayload.state = PAYLOAD_RUNNING;
                }
            }
            break;
        }
        case PAYLOAD_SEND_END_FILE:
        {   
            PAYLOAD_sendEndTransfer();   
            Serial.println("Payload experiment transfer complete");
            FILE_writeMetadata(&hpayload.experiment_file);
            COMMS_sendEndTransfer();
            hpayload.prev_state = PAYLOAD_SEND_END_FILE;
            hpayload.state      = PAYLOAD_WAIT_ACK;
            break;
        }
        case PAYLOAD_RUNNING:
        {
            Serial.println("RUNNING PAYLOAD...");
            break;
        }
        case PAYLOAD_GET_RESULTS:
        {
            break;
        }
        case PAYLOAD_SEND_ACK:
        {
            break;
        }

        case PAYLOAD_END:
        {
            break;
        }
        default:
            break;
        }

    return;
}

bool PAYLOAD_getStartCMD(void)
{
    UART_msg_t msg;
    if (UART_receive(hpayload.serial, &msg, DEFAULT_UART_TIMEOUT_US))
    {
        if (msg.length < 1)
        {
            Serial.println("Bad start message length");
            return false;
        }

        if ((msg.id == PAYLOAD_START_ID))
        {
            return true;
        }
        else
        {
            Serial.println("Warning: Bad command received from payload board.");
            return false;
        }
    }
    return false;
}



bool PAYLOAD_sendChunk(uint8_t id, const uint8_t *payload, uint8_t length)
{
    UART_msg_t msg = {0};
    msg.sof    = UART_SOF;
    msg.id     = id;
    msg.length = length;
    if (length > 0 && payload != NULL)
    {
        memcpy(msg.payload, payload, length);
    }
    UART_transmit(hpayload.serial, &msg);
    return true;
}

bool PAYLOAD_sendAck(void)
{
    UART_msg_t msg = {0};

    msg.sof    = UART_SOF;
    msg.id     = PAYLOAD_ACK_ID;
    msg.length = 0;

    UART_transmit(hpayload.serial, &msg);

    return true;
}
bool PAYLOAD_getAck(void)
{
    UART_msg_t msg;
    if (UART_receive(hpayload.serial, &msg, DEFAULT_UART_TIMEOUT_US))
    {
        if (msg.length < 1)
        {
            Serial.println("Bad ack message length");
            return false;
        }

        if ((msg.id == PAYLOAD_ACK_ID))
        {
            return true;
        }
        else
        {
            Serial.println("Warning: Bad command received from payload board.");
            return false;
        }
    }
    return false;
}



bool PAYLOAD_sendEndTransfer(void)
{
    UART_msg_t msg = {0};

    msg.sof = UART_SOF;
    msg.id  = PAYLOAD_END_TRANSFER_ID;
    msg.length = 1;
    msg.payload[0] = PAYLOAD_END_TRANSFER_ID; 

    UART_transmit(&Serial3, &msg);
    return true;
}