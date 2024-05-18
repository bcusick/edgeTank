////MASTER

#include <Wire.h>

// Define the structure for sensor values
struct SensorValues_t {
  float temperature;
  float humidity;
  int pressure;
};

// Function to send SensorValues_t structure
void sendSensorValues(const SensorValues_t &values) {
  Wire.beginTransmission(9); // Slave address
  Wire.write((byte *)&values, sizeof(values));
  Wire.endTransmission();
}

void setup() {
  Wire.begin(); // Start I2C communication
}

void loop() {
  // Sample sensor data
  SensorValues_t sensorData;
  sensorData.temperature = 25.5;
  sensorData.humidity = 50.5;
  sensorData.pressure = 1013;

  // Send sensor data
  sendSensorValues(sensorData);

  delay(1000); // Adjust delay as needed
}

////SLAVE

#include <Wire.h>

// Define the structure for sensor values
struct SensorValues_t {
  float temperature;
  float humidity;
  int pressure;
};

// Function to receive SensorValues_t structure
void receiveSensorValues(SensorValues_t &values) {
  Wire.requestFrom(8, sizeof(values)); // Master's address and size of data
  byte *ptr = (byte *)&values;
  for (int i = 0; i < sizeof(values); i++) {
    *ptr++ = Wire.read();
  }
}

void setup() {
  Wire.begin(8);                // Start I2C communication with address 8
  Wire.onRequest(requestEvent); // Register event
}

void loop() {
  delay(100); // Adjust delay as needed
}

// Callback function for request event
void requestEvent() {
  SensorValues_t sensorData;
  // Get sensor data (e.g., from sensors connected to this Arduino)
  // Populate sensorData with actual sensor readings
  // For now, let's just send some dummy values
  sensorData.temperature = 25.5;
  sensorData.humidity = 50.5;
  sensorData.pressure = 1013;

  // Send sensor data to master
  Wire.write((byte *)&sensorData, sizeof(sensorData));
}