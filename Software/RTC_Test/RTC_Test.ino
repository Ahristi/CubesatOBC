#include <Wire.h>

#define RV3028_ADDR 0x52

uint8_t decToBcd(uint8_t val)
{
    return ((val / 10) << 4) | (val % 10);
}

uint8_t bcdToDec(uint8_t val)
{
    return ((val >> 4) * 10) + (val & 0x0F);
}

void setRTC(
    uint8_t year,    // 0-99, e.g. 26 for 2026
    uint8_t month,   // 1-12
    uint8_t date,    // 1-31
    uint8_t weekday, // 0-6, Sunday = 0
    uint8_t hours,   // 0-23 UTC
    uint8_t minutes,
    uint8_t seconds
)
{
    Wire.beginTransmission(RV3028_ADDR);
    Wire.write(0x00);                 // start at seconds register

    Wire.write(decToBcd(seconds));
    Wire.write(decToBcd(minutes));
    Wire.write(decToBcd(hours));
    Wire.write(decToBcd(weekday));
    Wire.write(decToBcd(date));
    Wire.write(decToBcd(month));
    Wire.write(decToBcd(year));

    Wire.endTransmission();
}

void readRTC()
{
    Wire.beginTransmission(RV3028_ADDR);
    Wire.write(0x00);
    Wire.endTransmission();

    Wire.requestFrom(RV3028_ADDR, 7);

    if (Wire.available() < 7)
    {
        Serial.println("RTC read failed");
        return;
    }

    uint8_t seconds = bcdToDec(Wire.read() & 0x7F);
    uint8_t minutes = bcdToDec(Wire.read() & 0x7F);
    uint8_t hours   = bcdToDec(Wire.read() & 0x3F);
    uint8_t weekday = bcdToDec(Wire.read() & 0x07);
    uint8_t date    = bcdToDec(Wire.read() & 0x3F);
    uint8_t month   = bcdToDec(Wire.read() & 0x1F);
    uint8_t year    = bcdToDec(Wire.read());

    Serial.print("20");
    if (year < 10) Serial.print("0");
    Serial.print(year);

    Serial.print("-");
    if (month < 10) Serial.print("0");
    Serial.print(month);

    Serial.print("-");
    if (date < 10) Serial.print("0");
    Serial.print(date);

    Serial.print(" ");

    if (hours < 10) Serial.print("0");
    Serial.print(hours);

    Serial.print(":");
    if (minutes < 10) Serial.print("0");
    Serial.print(minutes);

    Serial.print(":");
    if (seconds < 10) Serial.print("0");
    Serial.print(seconds);

    Serial.println(" UTC");
}

void setup()
{
    Serial.begin(115200);
    while (!Serial);

    Wire.begin();

    Wire.beginTransmission(RV3028_ADDR);
    if (Wire.endTransmission() != 0)
    {
        Serial.println("RV-3028 not detected");
        while (1);
    }

    Serial.println("RV-3028 detected");

    // Set this to the correct UTC time.
    // Example: Friday 8 May 2026, 09:00:00 UTC
    // weekday: Sunday=0, Monday=1, ..., Friday=5
    //setRTC(26, 5, 8, 5, 11, 39, 30);

    Serial.println("RTC time set");
}

void loop()
{
    readRTC();
    delay(1000);
}