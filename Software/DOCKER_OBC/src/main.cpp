#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "scheduler.h"




void setup() {

    //Begin Peripherals
    Serial.begin(115200);
    Wire.begin();
    SPI.begin();
    
    LOGGING_Init();
    Scheduler_init();
}

void loop()
{
    Scheduler_run();
}