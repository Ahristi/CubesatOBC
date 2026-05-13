#ifndef PACKET_H
#define PACKET_H
#include <stdint.h>
#include <Arduino.h>    

//--------------Defines----------------
#define RX_CRC_BYTES      2
#define RX_HEADER_BYTES   3
#define RX_BUFFER_BYTES 256
#define UART_SOF       0x64


//--------------Structs and Enums----------------
typedef enum{
    UART_RX_WAIT_SOF,
    UART_RX_GET_ID,
    UART_RX_GET_LENGTH,
    UART_RX_READ_PAYLOAD,
    UART_RX_READ_CRC,
    UART_RX_CHECK_CRC
}UART_rx_state_t;

typedef struct{

}UART_handler_t;

typedef struct{
    uint8_t id;
    uint8_t sof;
    uint8_t length;
    uint8_t* payload;
    uint16_t crc;
}UART_msg_t;

//--------------Function Prototypes----------------
uint16_t UART_crc16_ccitt(const uint8_t *data, uint16_t length);
uint16_t UART_checkCRC(UART_msg_t* msg);
bool UART_getMessage(UART_msg_t* msg);


#endif