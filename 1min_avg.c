const int sensorPin = A0;  // Analog input pin that the sensor is attached to
const int numReadings = 60;  // Number of readings to store (one reading per second for a minute)

int readings[numReadings];  // Array to store the readings
int readIndex = 0;  // The index of the current reading
long total = 0;  // Total of the readings
unsigned long lastReadTime = 0;  // Last time we took a reading
unsigned long lastPrintTime = 0;  // Last time we printed the average

void setup() {
  Serial.begin(9600);  // Initialize serial communication
  // Initialize all the readings to 0
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
  }
}

void loop() {
  unsigned long currentTime = millis();

  // Check if a second has passed since the last reading
  if (currentTime - lastReadTime >= 1000) {
    lastReadTime = currentTime;

    // Read the sensor
    readings[readIndex] = analogRead(sensorPin);

    // Add the new reading to the total
    total = total + readings[readIndex];

    // Advance to the next position in the array
    readIndex = (readIndex + 1) % numReadings;
  }

  // Check if a minute has passed since the last print
  if (currentTime - lastPrintTime >= 60000) {
    lastPrintTime = currentTime;

    // Calculate the average
    float average = total / (float)numReadings;

    // Print the average to the serial monitor
    Serial.print("Average reading: ");
    Serial.println(average);
  }
}
