#ifndef ADCS_H
#define ADCS_H

//-------------Defines-------------
#define DETUMBLE_RATE_THRESHOLD 0.1 
#define DETUMBLE_SCALE_START 10.0f
#define ADCS_BAUDRATE 115200


//-------------Typedef and Enums-------------
typedef struct {
    float detumble_scale;


    //Attitude
    float roll;
    float pitch;
    float yaw;

    //Angular Velocity
    float roll_dot;
    float pitch_dot;
    float yaw_dot;

    //Angular Acceleration
    float roll_ddot;
    float pitch_ddot;
    float yaw_ddot;

    bool detumble_command_ready;
    bool pointing_command_ready;

}ADCS_Handler_t;


//-------------Variables-------------
extern ADCS_Handler_t hadcs;


//-------------Function Prototypes-------------
void ADCS_Init();
void ADCS_task();



#endif