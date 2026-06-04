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
    Wire.write(0x00); // start at seconds register

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

    Serial.print(" UTC, weekday=");
    Serial.println(weekday);
}

bool validRTCValues(
    int year,
    int month,
    int date,
    int weekday,
    int hours,
    int minutes,
    int seconds
)
{
    if (year < 0 || year > 99) return false;
    if (month < 1 || month > 12) return false;
    if (date < 1 || date > 31) return false;
    if (weekday < 0 || weekday > 6) return false;
    if (hours < 0 || hours > 23) return false;
    if (minutes < 0 || minutes > 59) return false;
    if (seconds < 0 || seconds > 59) return false;

    return true;
}

void handleSerialCommand()
{
    if (!Serial.available())
    {
        return;
    }

    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.length() == 0)
    {
        return;
    }

    int year, month, date, weekday, hours, minutes, seconds;

    int parsed = sscanf(
        command.c_str(),
        "SET %d %d %d %d %d %d %d",
        &year,
        &month,
        &date,
        &weekday,
        &hours,
        &minutes,
        &seconds
    );

    if (parsed != 7)
    {
        Serial.println("Invalid command.");
        Serial.println("Use:");
        Serial.println("SET yy mm dd weekday hh mm ss");
        Serial.println("Example:");
        Serial.println("SET 26 6 4 4 2 46 23");
        return;
    }

    if (!validRTCValues(year, month, date, weekday, hours, minutes, seconds))
    {
        Serial.println("Invalid RTC values.");
        Serial.println("Ranges:");
        Serial.println("year: 0-99");
        Serial.println("month: 1-12");
        Serial.println("date: 1-31");
        Serial.println("weekday: 0-6, Sunday=0");
        Serial.println("hours: 0-23");
        Serial.println("minutes: 0-59");
        Serial.println("seconds: 0-59");
        return;
    }

    setRTC(
        (uint8_t)year,
        (uint8_t)month,
        (uint8_t)date,
        (uint8_t)weekday,
        (uint8_t)hours,
        (uint8_t)minutes,
        (uint8_t)seconds
    );

    Serial.println("RTC time set.");
    readRTC();
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

    Serial.println("Enter UTC time using:");
    Serial.println("SET yy mm dd weekday hh mm ss");
    Serial.println("Example:");
    Serial.println("SET 26 6 4 4 2 46 23");
}

void loop()
{
    handleSerialCommand();

    static uint32_t lastPrint = 0;

    if (millis() - lastPrint >= 1000)
    {
        lastPrint = millis();
        readRTC();
    }
}