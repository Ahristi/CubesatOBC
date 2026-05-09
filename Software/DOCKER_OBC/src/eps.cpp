#include "eps.h"
#include <Arduino.h>

void EPS_task()
{
    CAN_message_t rxFrame;
    while (CAN_read(&rxFrame)) {
        switch (rxFrame.id)
        {
        case EPS_3V3_TELEMETRY_ID:
            EPS_RailTelemHandle(&EPS_telemetry.rail_3v3,rxFrame.buf,rxFrame.len);
            break;
        case EPS_5V_TELEMETRY_ID:
            EPS_RailTelemHandle(&EPS_telemetry.rail_5v,rxFrame.buf,rxFrame.len);
            break;
        case EPS_6V_TELEMETRY_ID:
            EPS_RailTelemHandle(&EPS_telemetry.rail_6v,rxFrame.buf,rxFrame.len);
            break;
        case EPS_12V_TELEMETRY_ID:
            EPS_RailTelemHandle(&EPS_telemetry.rail_12v,rxFrame.buf,rxFrame.len);
            break;
        case EPS_BMS_TELEMETRY_ID:
            EPS_BMSTelemHandle(rxFrame.buf, rxFrame.len);
            break;
        case EPS_SYS_TELEMETRY_ID:
            EPS_SYSTelemHandle(rxFrame.buf, rxFrame.len);
            break;
        default:
            break;
        }
    }
    
}

void EPS_RailTelemHandle(RailTelemetry_t* railTelem, uint8_t* buf, uint8_t len)
{
    if (len < LEN_RAIL_TELEM)
    {
        return;
    }
    else
    {
        railTelem->voltage    = buf[0]<< 8 |  buf[1];
        railTelem->current_ch1 = buf[2]<< 8 |  buf[3];
        railTelem->current_ch2 = buf[4]<< 8 |  buf[5];
    }
}
void EPS_BMSTelemHandle(uint8_t* buf, uint8_t len)
{
    if (len < LEN_BMS_TELEM)
    {
        return;
    }
    else
    {
        EPS_telemetry.battery_voltage  = buf[0]<< 8 |  buf[1];
        EPS_telemetry.battery_current  = buf[2]<< 8 |  buf[3];
        EPS_telemetry.sys_voltage      = buf[4]<< 8 |  buf[5];
        EPS_telemetry.battery_temp     = buf[6];
        EPS_telemetry.charger_die_temp = buf[7];
    }
}
void EPS_SYSTelemHandle(uint8_t* buf, uint8_t len)
{
    if (len < LEN_SYS_TELEM)
    {
        return;
    }
    else
    {
        EPS_telemetry.eFuse_states     = buf[0];
        EPS_telemetry.eFuse_faults     = buf[1];
        EPS_telemetry.mcu_temp         = buf[2];
    }
}

void EPS_printRailTelemetry(const char* name, const RailTelemetry_t* rail)
{
    Serial.print(name);
    Serial.print(" | V=");
    Serial.print(rail->voltage);

    Serial.print(" | I1=");
    Serial.print(rail->current_ch1);

    Serial.print(" | I2=");
    Serial.println(rail->current_ch2);
}


void EPS_debugPrint(void)
{
    static elapsedMillis printTimer;

    // Print once per second
    if (printTimer < 1000) {
        return;
    }
    printTimer = 0;

    Serial.println();
    Serial.println("========== EPS TELEMETRY ==========");

    EPS_printRailTelemetry("3V3 ", &EPS_telemetry.rail_3v3);
    EPS_printRailTelemetry("5V  ", &EPS_telemetry.rail_5v);
    EPS_printRailTelemetry("6V  ", &EPS_telemetry.rail_6v);
    EPS_printRailTelemetry("12V ", &EPS_telemetry.rail_12v);

    Serial.println("-----------------------------------");

    Serial.print("Battery Voltage: ");
    Serial.println(EPS_telemetry.battery_voltage);

    Serial.print("System Voltage:  ");
    Serial.println(EPS_telemetry.sys_voltage);

    Serial.print("Battery Current: ");
    Serial.println(EPS_telemetry.battery_current);

    Serial.print("Battery Temp:    ");
    Serial.println(EPS_telemetry.battery_temp);

    Serial.print("MCU Temp:        ");
    Serial.println(EPS_telemetry.mcu_temp);

    Serial.print("Charger Die Temp:");
    Serial.println(EPS_telemetry.charger_die_temp);

    Serial.println("-----------------------------------");

    Serial.print("MPPT1 Voltage:   ");
    Serial.println(EPS_telemetry.mppt1_voltage);

    Serial.print("MPPT2 Voltage:   ");
    Serial.println(EPS_telemetry.mppt2_voltage);

    Serial.print("MPPT1 Current:   ");
    Serial.println(EPS_telemetry.mppt1_current);

    Serial.print("MPPT2 Current:   ");
    Serial.println(EPS_telemetry.mppt2_current);

    Serial.println("-----------------------------------");

    Serial.print("eFuse States:    0b");
    for (int8_t i = 7; i >= 0; i--) {
        Serial.print((EPS_telemetry.eFuse_states >> i) & 0x01);
    }
    Serial.println();

    Serial.print("eFuse Faults:    0b");
    for (int8_t i = 7; i >= 0; i--) {
        Serial.print((EPS_telemetry.eFuse_faults >> i) & 0x01);
    }
    Serial.println();

    Serial.println("===================================");
}


