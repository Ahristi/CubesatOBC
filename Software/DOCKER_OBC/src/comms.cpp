#include "comms.h"
#include <Arduino.h>


COMMS_Handler_t hcomms;

void COMMS_Init(void)
{
    Serial3.begin(COMMS_BAUDRATE);
    Serial.println("COMMS UART initialised on Serial3");
}

void COMMS_task()
{
    hcomms.beacon_tick++;
    if (hcomms.beacon_tick >= BEACON_TICK_OC)
    {

    }

    return;
}


void COMMS_sendBeacon(void)
{



}