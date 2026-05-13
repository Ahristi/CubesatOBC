#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <FlexCAN_T4.h>

#include "scheduler.h"
#include "can.h"
#include "adcs.h"
#include "eps.h"
#include "comms.h"
#include "system.h"



void setup() {
    //Begin Peripherals
    Serial.begin(115200);
    Wire.begin();
    SPI.begin();

    CAN_Init();
    COMMS_Init();
    LOGGING_Init();
    Scheduler_init();
    EPS_Init();
    ADCS_Init();
    SYSTEM_Init();
}

void loop()
{
    Scheduler_run();
}