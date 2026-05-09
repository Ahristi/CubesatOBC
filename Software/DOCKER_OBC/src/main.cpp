#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <FlexCAN_T4.h>

#include "scheduler.h"
#include "can.h"



void setup() {

    //Begin Peripherals
    Serial.begin(115200);
    Wire.begin();
    SPI.begin();
    Can0.begin();
    Can0.setBaudRate(125000);
    Can0.setMaxMB(16);
    Can0.enableLoopBack(true);  
    //Configure GPIO Pins
    pinMode(CAN_VIO_PIN, OUTPUT);
    
    LOGGING_Init();
    Scheduler_init();
}

void loop()
{
    Scheduler_run();
}