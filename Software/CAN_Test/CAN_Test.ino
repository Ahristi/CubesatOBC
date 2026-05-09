#include <FlexCAN_T4.h>
#define CAN_VIO_PIN 24
FlexCAN_T4<CAN3, RX_SIZE_256, TX_SIZE_16> Can0;

elapsedMillis txTimer;

void setup() {
  Serial.begin(115200);
  pinMode(CAN_VIO_PIN, OUTPUT);
  digitalWrite(CAN_VIO_PIN, HIGH);
  while (!Serial && millis() < 3000);

  Serial.println("Starting CAN3 test @ 125 kbps");

  Can0.begin();
  Can0.setBaudRate(125000);

  Serial.println("Sending CAN frame every 1 second...");
}

void loop() {

  // Transmit every second
  if (txTimer > 1000) {
    txTimer = 0;

    CAN_message_t txMsg;

    txMsg.id = 0x123;
    txMsg.len = 8;

    txMsg.buf[0] = 0x11;
    txMsg.buf[1] = 0x22;
    txMsg.buf[2] = 0x33;
    txMsg.buf[3] = 0x44;
    txMsg.buf[4] = 0x55;
    txMsg.buf[5] = 0x66;
    txMsg.buf[6] = 0x77;
    txMsg.buf[7] = 0x88;

    if (Can0.write(txMsg)) {
      Serial.println("TX OK");
    } else {
      Serial.println("TX FAIL");
    }
  }

  // Receive messages
  CAN_message_t rxMsg;

  while (Can0.read(rxMsg)) {

    Serial.print("RX ID: 0x");
    Serial.print(rxMsg.id, HEX);

    Serial.print(" LEN: ");
    Serial.print(rxMsg.len);

    Serial.print(" DATA: ");

    for (uint8_t i = 0; i < rxMsg.len; i++) {
      if (rxMsg.buf[i] < 0x10) Serial.print("0");
      Serial.print(rxMsg.buf[i], HEX);
      Serial.print(" ");
    }

    Serial.println();
  }
}