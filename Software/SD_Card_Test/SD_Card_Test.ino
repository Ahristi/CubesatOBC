#include <SPI.h>
#include <SD.h>

#define SD_CS_PIN 10

File testFile;

void setup()
{
    Serial.begin(115200);
    while (!Serial);

    Serial.println("SD Card Test");

    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);

    SPI.begin();

    if (!SD.begin(SD_CS_PIN))
    {
        Serial.println("SD card initialisation failed!");

        while (1)
        {
            delay(100);
        }
    }

    Serial.println("SD card detected");

    // Write test
    testFile = SD.open("test.txt", FILE_WRITE);

    if (testFile)
    {
        Serial.println("Writing to test.txt...");

        testFile.println("SD card write test successful");
        testFile.close();

        Serial.println("Write complete");
    }
    else
    {
        Serial.println("Failed to open file for writing");
    }

    // Read test
    testFile = SD.open("test.txt", FILE_READ);

    if (testFile)
    {
        Serial.println("Reading test.txt:");

        while (testFile.available())
        {
            Serial.write(testFile.read());
        }

        testFile.close();

        Serial.println("\nRead complete");
    }
    else
    {
        Serial.println("Failed to open file for reading");
    }
}

void loop()
{
}