#include <SPI.h>
#include <SD.h>
#include <Arduino_EdgeControl.h>

// Define the chip select pin for the SD card module
const int chipSelect = PIN_SD_CS;

// Define sensor pins
//TODO

File dataFile;


// Sensor data structure
struct SensorData {
  int timestamp;
  int tank1;
  int tank2;
  int flowMain;

};

void setup() {
  Serial.begin(9600);

  EdgeControl.begin();
  // Power on the 3V3 rail for SD Card
  Power.on(PWR_3V3);

  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialized.");

  

  // Open the file. If it doesn't exist, create it.
  dataFile = SD.open("datalog.csv", FILE_WRITE);
  
  // If the file opened successfully, write the header
  if (dataFile) {
    dataFile.println("Time,Sensor1,Sensor2,Sensor3");
    dataFile.close();
  } else {
    Serial.println("Error opening datalog.csv");
  }
}

void loop() {
  


  // Read sensor data
  readSensors();
  
  // Log to SD card
  logDataToSD(data);

  // Wait for a second before the next loop
  delay(1000);
}

void readSensors() {
  SensorData data;
  data.tank1 = readTemperature(); // Replace with actual function
  data.tank2 = readHumidity(); // Replace with actual function
  data.flowMain = readPressure(); // Replace with actual function
  data.timestamp = time(NULL);
}

///TBD
float readTemperature() {
    // Read temperature from sensor and return
}

float readHumidity() {
    // Read humidity from sensor and return
}


void logDataToSD(const SensorData &data) {
    File dataFile = SD.open("datalog.txt", FILE_WRITE);
    if (dataFile) {
        dataFile.print(data.timestamp);
        dataFile.print(",");
        dataFile.print(data.tank1);
        dataFile.print(",");
        dataFile.println(data.tank2);
        dataFile.print(",");
        dataFile.println(data.flowMain);
        dataFile.close();
    } else {
        Serial.println("Error opening datalog.txt");
    }
}
