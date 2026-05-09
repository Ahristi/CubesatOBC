#ifndef CAN_H
#define CAN_H
#include <FlexCAN_T4.h>



//-------------Defines-------------
#define CAN_VIO_PIN 24 //Connected to VIO on TCAN3413. Needs to be high for IC to work.
#define CAN_BAUD_RATE 125000
#define CAN_MAX_MB 16
CA

//-------------Function Prototypes-------------
void CAN_Init(void);


//-------------Variables-------------
FlexCAN_T4<CAN3, RX_SIZE_256, TX_SIZE_16> Can0;


#endif