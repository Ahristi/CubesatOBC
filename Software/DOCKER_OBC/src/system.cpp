#include "system.h"

SYSTEM_Handler_t hsys{INIT,INIT, true, LONG_PAUSE, 0, 0};

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
    SYSTEM_stateMachine();
    //Debug
    //EPS_debugPrint();
    //SYSTEM_debugPrint();
    //ADCS_debugPrint();
    //Scheduler_debugPrint();
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

void SYSTEM_stateMachine(void)
{
    switch (hsys.state)
    {
        case INIT:
        {
            if (SYSTEM_isEnteringState())
            {
                EPS_enableEFuse(EPS_EFUSE_5V_CH1);   // Enable ADCS
                //EPS_enableEFuse(EPS_EFUSE_12V_CH2);  // Enable comms board
                heps.eFuse_msg_ready = true;
            }
            if ((EPS_telemetry.eFuse_states & heps.eFuse_states) == heps.eFuse_states)
            {
                SYSTEM_setState(DETUMBLE);
            }
            else
            {
                EPS_enableEFuse(EPS_EFUSE_5V_CH1);   // Retry Enable ADCS
                //EPS_enableEFuse(EPS_EFUSE_12V_CH2);  // Retry Enable comms board
                heps.eFuse_msg_ready = true;
            }
            break;
        }
        case DETUMBLE:
        {
            if (SYSTEM_isEnteringState())
            {
                //Nothing
            }
            if (hadcs.detumble_scale < DETUMBLE_RATE_THRESHOLD)
            {
                SYSTEM_setState(IDLE);
            }
            break;
        }
        case IDLE:
        {
            if (SYSTEM_isEnteringState())
            {
                //Nothing
            }
            if (hcomms.state != COMMS_IDLE)
            {   
                SYSTEM_setState(LINK);
            }
            else if (hpayload.experiment_ready)
            {
                SYSTEM_setState(EXPERIMENT);
            }
            break;
        }
        case EXPERIMENT:
        {
            if (SYSTEM_isEnteringState())
            {
                hpayload.start_experiment = true;
                EPS_enableEFuse(EPS_EFUSE_6V_CH1);   // Enable payload
            }
            if ((EPS_telemetry.eFuse_states & heps.eFuse_states) != heps.eFuse_states)
            {
                EPS_enableEFuse(EPS_EFUSE_6V_CH1);   // Retry Enable payload
                heps.eFuse_msg_ready = true;
            }   
            if (hpayload.experiment_finished)
            {
                EPS_disableEFuse(EPS_EFUSE_6V_CH1);   // Turn off payload
                heps.eFuse_msg_ready = true;
                SYSTEM_setState(IDLE);
            }
            break;
        }
        case LINK:
        {
            if (SYSTEM_isEnteringState())
            {
                // Optional: enable radio / start downlink
            }

            // Handle comms link
            break;
        }
        default:
        {
            SYSTEM_setState(INIT);
            break;
        }
    }
}

void SYSTEM_setState(SYSTEM_State_t new_state)
{
    if (hsys.state != new_state)
    {
        hsys.previous_state = hsys.state;
        hsys.state = new_state;
        hsys.state_entry = true;
    }
}
bool SYSTEM_isEnteringState(void)
{
    if (hsys.state_entry)
    {
        hsys.state_entry = false;
        return true;
    }

    return false;
}

void SYSTEM_debugPrint(void)
{
    static elapsedMillis printTimer;

    // Print once per second
    if (printTimer < 1000) {
        return;
    }
    printTimer = 0;

    Serial.println();
    Serial.println("========== SYSTEM DEBUG ==========");

    Serial.print("State: ");
    switch (hsys.state)
    {
    case INIT:
        Serial.println("INIT");
        break;

    case DETUMBLE:
        Serial.println("DETUMBLE");
        break;

    case IDLE:
        Serial.println("IDLE");
        break;

    case EXPERIMENT:
        Serial.println("EXPERIMENT");
        break;

    case LINK:
        Serial.println("LINK");
        break;

    default:
        Serial.println("UNKNOWN");
        break;
    }

    Serial.print("State Entry: ");
    Serial.println(hsys.state_entry ? "true" : "false");

    Serial.print("Watchdog Tick Count: ");
    Serial.println(hsys.watchdog_tick_count);

    Serial.println("----------------------------------");

    Serial.print("ADCS Omega:  ");
    Serial.println(hadcs.telemetry.omega_x, 6);

    Serial.print("ADCS Pitch Rate: ");
    Serial.println(hadcs.telemetry.omega_y, 6);

    Serial.print("ADCS Yaw Rate:   ");
    Serial.println(hadcs.telemetry.omega_z, 6);


    Serial.println("----------------------------------");

    Serial.print("Desired eFuse States: 0b");
    for (int8_t i = 7; i >= 0; i--) {
        Serial.print((heps.eFuse_states >> i) & 0x01);
    }
    Serial.println();

    Serial.print("Actual eFuse States:  0b");
    for (int8_t i = 7; i >= 0; i--) {
        Serial.print((EPS_telemetry.eFuse_states >> i) & 0x01);
    }
    Serial.println();

    Serial.print("eFuse Msg Ready: ");
    Serial.println(heps.eFuse_msg_ready ? "true" : "false");

    Serial.print("Converter States: 0b");
    for (int8_t i = 7; i >= 0; i--) {
        Serial.print((heps.converter_states >> i) & 0x01);
    }
    Serial.println();

    Serial.println("==================================");
}