#include <Wire.h>

#define RV3028_ADDR 0x52

// RV-3028 registers
#define REG_STATUS   0x0E
#define REG_CTRL1    0x0F
#define REG_EEADDR   0x25
#define REG_EEDATA   0x26
#define REG_EECMD    0x27

// EEPROM register
#define EEPROM_BACKUP 0x37

// Bits
#define STATUS_EEBUSY 0x80
#define CTRL1_EERD    0x08

uint8_t readReg(uint8_t reg)
{
    Wire.beginTransmission(RV3028_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);

    Wire.requestFrom(RV3028_ADDR, 1);
    return Wire.read();
}

void writeReg(uint8_t reg, uint8_t val)
{
    Wire.beginTransmission(RV3028_ADDR);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

bool waitEEBusyClear()
{
    uint32_t start = millis();

    while (readReg(REG_STATUS) & STATUS_EEBUSY)
    {
        if (millis() - start > 500)
            return false;

        delay(5);
    }

    return true;
}

bool writeEEPROM(uint8_t eeAddr, uint8_t data)
{
    if (!waitEEBusyClear())
        return false;

    // Disable automatic EEPROM refresh
    uint8_t ctrl1 = readReg(REG_CTRL1);
    writeReg(REG_CTRL1, ctrl1 | CTRL1_EERD);

    delay(1);

    writeReg(REG_EEADDR, eeAddr);
    writeReg(REG_EEDATA, data);

    // EEPROM write command sequence
    writeReg(REG_EECMD, 0x00);
    writeReg(REG_EECMD, 0x21);

    if (!waitEEBusyClear())
        return false;

    // Re-enable automatic EEPROM refresh
    ctrl1 = readReg(REG_CTRL1);
    writeReg(REG_CTRL1, ctrl1 & ~CTRL1_EERD);

    return true;
}

uint8_t readEEPROM(uint8_t eeAddr)
{
    waitEEBusyClear();

    uint8_t ctrl1 = readReg(REG_CTRL1);
    writeReg(REG_CTRL1, ctrl1 | CTRL1_EERD);

    delay(1);

    writeReg(REG_EEADDR, eeAddr);

    // EEPROM read command sequence
    writeReg(REG_EECMD, 0x00);
    writeReg(REG_EECMD, 0x22);

    waitEEBusyClear();

    uint8_t data = readReg(REG_EEDATA);

    ctrl1 = readReg(REG_CTRL1);
    writeReg(REG_CTRL1, ctrl1 & ~CTRL1_EERD);

    return data;
}

void setup()
{
    Serial.begin(115200);
    while (!Serial);

    Wire.begin();

    Serial.println("RV-3028 backup switchover config");

    Wire.beginTransmission(RV3028_ADDR);
    if (Wire.endTransmission() != 0)
    {
        Serial.println("RV-3028 not detected");
        while (1);
    }

    Serial.println("RV-3028 detected");

    uint8_t before = readEEPROM(EEPROM_BACKUP);

    Serial.print("EEPROM Backup before: 0x");
    Serial.println(before, HEX);

    /*
        EEPROM Backup register 0x37:

        bit 7    FEDE = 1  fast edge detection enabled
        bit 5    TCE  = 1  trickle charger enabled
        bits 3:2 BSM  = 11 level switching mode

        Value:
        0b10101100 = 0xAC
    */

    uint8_t backupConfig = 0xAC;

    if (writeEEPROM(EEPROM_BACKUP, backupConfig))
    {
        Serial.println("EEPROM write complete");
    }
    else
    {
        Serial.println("EEPROM write failed");
        while (1);
    }

    delay(100);

    uint8_t after = readEEPROM(EEPROM_BACKUP);

    Serial.print("EEPROM Backup after: 0x");
    Serial.println(after, HEX);

    if (after == backupConfig)
    {
        Serial.println("Backup switchover enabled: Level Switching Mode");
        Serial.println("Trickle charger Enabled");
    }
    else
    {
        Serial.println("Readback mismatch");
    }
}

void loop()
{
}