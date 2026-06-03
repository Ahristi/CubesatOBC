#include "adcs.h"
#include <Arduino.h>
 
ADCS_Handler_t hadcs;

void ADCS_Init(void)
{
    hadcs.detumble_scale           = DETUMBLE_SCALE_START;
    hadcs.attitude_command_ready   = false;
    hadcs.orbital_parameters_ready = false;
    hadcs.detumble_command_ready   = false;
    hadcs.pointing_command_ready   = false;
    Serial1.begin(ADCS_BAUDRATE);
    Serial.println("ADCS UART initialised on Serial1");
}

void ADCS_task(void)
{
    ADCS_getTelemetry();
    ADCS_telemetryHandle();
    ADCS_updateAttitude(); 
    ADCS_updateOrbitalParameters();
    return;
}

void ADCS_getTelemetry(void)
{
    UART_msg_t msg;
    if (UART_receive(&Serial1, &msg, DEFAULT_UART_TIMEOUT_US))
    {
        ADCS_processPacket(msg.id, msg.payload, msg.length);
    }
}

void ADCS_processPacket(uint8_t id, uint8_t *payload, uint8_t payload_length)
{
    uint8_t expected_length = ADCS_getRxPayloadLength(id);
    if (payload_length != expected_length)
    {
        Serial.print("Bad ADCS packet length. RX=");
        Serial.print(payload_length);
        Serial.print(" Expected=");
        Serial.println(expected_length);
        return;
    }
    if (id == ADCS_PACKET_TELEMETRY)
    {
        if (payload_length < sizeof(ADCS_TelemetryPacket_t))
        {
            Serial.println("ADCS telemetry packet too short");
            return;
        }
        memcpy(&hadcs.telemetry, payload, sizeof(ADCS_TelemetryPacket_t));
        Serial.println("ADCS telemetry updated");
    }
}

uint8_t ADCS_getRxPayloadLength(uint8_t id) 
{
    switch (id)
    {
        case ADCS_PACKET_TELEMETRY:
            return sizeof(ADCS_TelemetryPacket_t);
        case ADCS_PACKET_ACK:
            return ADCS_PACKET_ACK_BYTES;
        case ADCS_PACKET_ERROR:
            return ADCS_PACKET_ERROR_BYTES;
        default:
            return 0;
    } 
}
void ADCS_updateAttitude(void)
{
    if (hadcs.attitude_command_ready)
    {
        UART_msg_t msg;
        hadcs.attitude_command_ready = false;
        msg.sof    = UART_SOF;
        msg.id     = ADCS_ATTITUDE_UPDATE_ID;
        msg.length = sizeof(ADCS_attitudeCommand_t);
        memcpy(msg.payload, &hadcs.attitude_command, sizeof(ADCS_attitudeCommand_t));
        UART_transmit(&Serial1, &msg);
        Serial.println("Sent attitude command");
    }
}
void ADCS_updateOrbitalParameters(void)
{
    if (hadcs.orbital_parameters_ready)
    {
        UART_msg_t msg;
        hadcs.orbital_parameters_ready =  false;
        msg.sof    = UART_SOF;
        msg.id     = ADCS_ORBIT_UPDATE_ID;
        msg.length = ADCS_ORBIT_UPDATE_BYTES;
        memcpy(msg.payload, &hadcs.orbital_parameters, sizeof(ADCS_orbitalParameters_t));
        UART_transmit(&Serial1, &msg);
    }
}
void ADCS_telemetryHandle(void)
{
    /*
        This is a weird way of doing it since we copy the same values between like 4 structs.
        I was thinking of binning the ADCS_telemetry struct since its effectively the same as hadcs.
        But I want it to be consistent with the way telemetry is handled with the EPS so just doing this.   
        Right now the data flows like this:
        UART_msg->hadcs->ADCS_telemetry->logging_record_t->SD Card/WOD.
    */
    ADCS_telemetry.roll           = hadcs.telemetry.roll;   
    ADCS_telemetry.pitch          = hadcs.telemetry.pitch;  
    ADCS_telemetry.yaw            = hadcs.telemetry.yaw; 
    ADCS_telemetry.omega_x        = hadcs.telemetry.omega_x;
    ADCS_telemetry.omega_y        = hadcs.telemetry.omega_y;
    ADCS_telemetry.omega_z        = hadcs.telemetry.omega_z;
    ADCS_telemetry.x_rw_speed     = hadcs.telemetry.rw1;    
    ADCS_telemetry.y_rw_speed     = hadcs.telemetry.rw2;    
    ADCS_telemetry.z_rw_speed     = hadcs.telemetry.rw3;    
    ADCS_telemetry.x_mag_current  = hadcs.telemetry.it1;    
    ADCS_telemetry.y_mag_current  = hadcs.telemetry.it2;    
    ADCS_telemetry.z_mag_current  = hadcs.telemetry.it3;    
    ADCS_telemetry.detumble_scale = hadcs.telemetry.detumble_scale;
    ADCS_telemetry.faults         = hadcs.telemetry.faults;
}





void ADCS_debugPrint(void)
{
    Serial.println("========== ADCS ==========");

    Serial.print("Detumble Scale: ");
    Serial.println(hadcs.telemetry.detumble_scale, 6);

    Serial.println();

    Serial.println("Attitude:");
    Serial.print("  Roll  : ");
    Serial.println(hadcs.telemetry.roll, 6);

    Serial.print("  Pitch : ");
    Serial.println(hadcs.telemetry.pitch, 6);

    Serial.print("  Yaw   : ");
    Serial.println(hadcs.telemetry.yaw, 6);

    Serial.println();

    Serial.println("Omega:");
    Serial.print("  Omega x  : ");
    Serial.println(hadcs.telemetry.omega_x, 6);

    Serial.print("  Omega y : ");
    Serial.println(hadcs.telemetry.omega_y, 6);

    Serial.print("  Omega z   : ");
    Serial.println(hadcs.telemetry.omega_z, 6);

    Serial.println();

    Serial.println("Reaction Wheels:");
    Serial.print("  R/W 1  : ");
    Serial.println(hadcs.telemetry.rw1, 6);

    Serial.print("  R/W 2  : ");
    Serial.println(hadcs.telemetry.rw2, 6);

    Serial.print("  R/W 3  : ");
    Serial.println(hadcs.telemetry.rw3, 6);


    Serial.println("Magnetorquers:");
    Serial.print("  Torquer 1  : ");
    Serial.println(hadcs.telemetry.it1, 6);

    Serial.print("  Torquer 2  : ");
    Serial.println(hadcs.telemetry.it1, 6);

    Serial.print("  Torquer 3  : ");
    Serial.println(hadcs.telemetry.it1, 6);

    Serial.println();

    Serial.println("Commands:");
    Serial.print("  Detumble Command Ready : ");
    Serial.println(hadcs.detumble_command_ready ? "TRUE" : "FALSE");

    Serial.print("  Pointing Command Ready : ");
    Serial.println(hadcs.pointing_command_ready ? "TRUE" : "FALSE");

    Serial.println("==========================");
}