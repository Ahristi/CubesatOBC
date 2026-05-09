#ifndef CAN_H
#define CAN_H
#include <FlexCAN_T4.h>
#include "eps.h"


//-------------Defines-------------
#define CAN_VIO_PIN 24 //Connected to VIO on TCAN3413. Needs to be high for IC to work.
#define CAN_BAUD_RATE 125000
#define CAN_MAX_MB 16



//-------------Typedefs and Enums-------------


//-------------Function Prototypes-------------
void CAN_Init(void);
bool CAN_send(uint32_t id, uint8_t* data, uint8_t len);
bool CAN_read(CAN_message_t* frame);



//-------------Variables-------------



#endif