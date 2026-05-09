#include "adcs.h"
#include <Arduino.h>

ADCS_Handler_t hadcs;

void ADCS_Init()
{
    hadcs.detumble_scale = DETUMBLE_SCALE_START;
    Serial4.begin(ADCS_BAUDRATE);
    Serial.println("ADCS UART initialised on Serial4");
}

void ADCS_task()
{
    while (Serial4.available())
    {
        uint8_t byte = Serial4.read();
        Serial.print("ADCS RX: 0x");
        if (byte < 0x10) Serial.print("0");
        Serial.println(byte, HEX);
    }
    
}
