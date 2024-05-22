#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>

// Define the chip select pin for the SD card module
const int chipSelect = 10;

// Sensor pins
const int tempSensorPin = A0;
const int humSensorPin = A1;
const int presSensorPin = A2;
const int lightSensorPin = A3;
const int soundSensorPin = A4;

File dataFile;

void setup() {
  Serial.begin(9600);

  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialized.");

  dataFile = SD.open("datalog.json", FILE_WRITE);
  if (!dataFile) {
    Serial.println("Failed to open file for writing!");
    return;
  }
  Serial.println("File opened for writing.");
}

void loop() {
  int tempValue = analogRead(tempSensorPin);
  int humValue = analogRead(humSensorPin);
  int presValue = analogRead(presSensorPin);
  int lightValue = analogRead(lightSensorPin);
  int soundValue = analogRead(soundSensorPin);

  StaticJsonDocument<256> jsonDoc;
  jsonDoc["timestamp"] = millis();

  JsonObject sensors = jsonDoc.createNestedObject("sensors");
  sensors["temperature"]["value"] = tempValue;
  sensors["temperature"]["unit"] = "C";
  sensors["humidity"]["value"] = humValue;
  sensors["humidity"]["unit"] = "%";
  sensors["pressure"]["value"] = presValue;
  sensors["pressure"]["unit"] = "hPa";
  sensors["light"]["value"] = lightValue;
  sensors["light"]["unit"] = "lux";
  sensors["sound"]["value"] = soundValue;
  sensors["sound"]["unit"] = "dB";

  String jsonString;
  serializeJson(jsonDoc, jsonString);

  Serial.println(jsonString);

  if (dataFile) {
    dataFile.println(jsonString);
    dataFile.flush();
    Serial.println("Data written to SD card.");
  } else {
    Serial.println("Error writing to file!");
  }

  delay(1000); // Wait for 1 second before logging again
}
