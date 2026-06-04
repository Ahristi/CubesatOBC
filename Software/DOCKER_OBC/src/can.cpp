#include "can.h"
FlexCAN_T4<CAN3, RX_SIZE_1024, TX_SIZE_16> Can0;


void CAN_Init()
{
    //Enable TCAN3413 IC
    pinMode(CAN_VIO_PIN, OUTPUT);
    digitalWrite(CAN_VIO_PIN, HIGH);

    //Configure CAN peripheral
    Can0.begin();
    Can0.setBaudRate(CAN_BAUD_RATE);
    Can0.setMaxMB(CAN_MAX_MB);
    Can0.setMBFilter(REJECT_ALL);

    // Configure RX mailboxes
    Can0.setMB(MB0, RX, STD);
    Can0.setMB(MB1, RX, STD);
    Can0.setMB(MB2, RX, STD);
    Can0.setMB(MB3, RX, STD);
    Can0.setMB(MB4, RX, STD);
    Can0.setMB(MB5, RX, STD);

    Can0.setMBFilter(MB0, EPS_3V3_TELEMETRY_ID);
    Can0.setMBFilter(MB1, EPS_5V_TELEMETRY_ID);
    Can0.setMBFilter(MB2, EPS_6V_TELEMETRY_ID);
    Can0.setMBFilter(MB3, EPS_12V_TELEMETRY_ID);
    Can0.setMBFilter(MB4, EPS_BMS_TELEMETRY_ID);
    Can0.setMBFilter(MB5, EPS_SYS_TELEMETRY_ID);


    // Configure TX mailboxes
    Can0.setMB(MB8, TX, STD);
    Can0.setMB(MB9, TX, STD);

    Can0.setMBFilter(MB0,
        EPS_3V3_TELEMETRY_ID,
        EPS_5V_TELEMETRY_ID,
        EPS_6V_TELEMETRY_ID,
        EPS_12V_TELEMETRY_ID
    );

    Can0.setMBFilter(MB1,
        EPS_BMS_TELEMETRY_ID,
        EPS_SYS_TELEMETRY_ID,
        EPS_MPPT_TELEMETRY_ID
    );

    Serial.println("CAN3 initialized");
}


bool CAN_send(uint32_t id, uint8_t* data, uint8_t len)
{
    if (len > 8) {
        return false;   
    }

    CAN_message_t msg;

    msg.id = id;
    msg.len = len;
    msg.flags.extended = 0;  

    for (uint8_t i = 0; i < len; i++) {
        msg.buf[i] = data[i];
    }

    return Can0.write(msg);
}

bool CAN_read(CAN_message_t* frame)
{
    if (frame == nullptr) {
        return false;
    }

    return Can0.read(*frame);
}