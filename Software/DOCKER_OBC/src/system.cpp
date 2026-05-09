#include "system.h"

SYSTEM_Handler_t hsys{LONG_PAUSE, 0, 0};

void SYSTEM_Init(void)
{
    pinMode(SYSTEM_LED0_PIN, OUTPUT); //D2 on the PCB
    pinMode(SYSTEM_LED1_PIN, OUTPUT); //D3 on the PCB
    digitalWrite(SYSTEM_LED0_PIN, HIGH);
    digitalWrite(SYSTEM_LED1_PIN, HIGH);
    hsys.stat_led_state = LONG_PAUSE;
}

void SYSTEM_task(void)
{
    SYSTEM_heartbeat();
    SYSTEM_watchdog();

    //Debug
    EPS_debugPrint();
}


/*
    State machine for heartbeat LED.
    Diode cathode is connected to GPIO so digitalwrite is inverted
*/
void SYSTEM_heartbeat(void)
{
    switch (hsys.stat_led_state)
    {
        case LONG_PAUSE:
            if (hsys.heartbeat_tick_count == LONG_PAUSE_TICKS)
            {
                hsys.heartbeat_tick_count = 0;
                hsys.stat_led_state = FIRST_PULSE;
                digitalWrite(SYSTEM_LED1_PIN, LOW);
            }
            else
            {
                hsys.heartbeat_tick_count++;
            }
            break;
        case FIRST_PULSE:
            hsys.stat_led_state = SHORT_PAUSE;
            digitalWrite(SYSTEM_LED1_PIN, HIGH);
            break;
        case SHORT_PAUSE:
            hsys.stat_led_state = SECOND_PULSE;
            digitalWrite(SYSTEM_LED1_PIN, LOW);
            break;
        case SECOND_PULSE:
            hsys.stat_led_state = LONG_PAUSE;
            digitalWrite(SYSTEM_LED1_PIN, HIGH);
            break;
        default:
            break;
    }
}
void SYSTEM_watchdog(void)
{
    if (hsys.watchdog_tick_count == WATHDOG_TICKS)
    {
        hsys.watchdog_tick_count = 0;
        uint32_t ID = OBC_WATCHDOG_ID;
        uint8_t data[1] = {OBC_WATCHDOG_CODE};
        CAN_send(ID, data, 1);
    }
    else
    {
        hsys.watchdog_tick_count++;
    }
}