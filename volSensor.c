//const int sensorPin = A0; // Analog pin connected to the ADC input
const float referenceVoltage = 3.3; // Reference voltage of the ADC
const float resistorValue = 220.0; // Resistor value in ohms

void setup() {
  Serial.begin(9600); // Initialize serial communication
}

void loop() {
  //int sensorValue = analogRead(sensorPin); // Read ADC value
  //float voltage = sensorValue * (referenceVoltage / 1023.0); // Convert ADC value to voltage
  
  // Calculate current using Ohm's Law (I = V / R)
  //float current = voltage / resistorValue * 1000.0; // Convert to mA

  float w_level = ((voltsReference / resistorValue * 1000.0) - 4.0) * 6.25;  //convert to ma,  x = (y - 4)/16 since 4 = 0 and 20 = max

  // Print the results
  Serial.print("ADC Value: ");
  Serial.print(sensorValue);
  Serial.print(", Voltage (V): ");
  Serial.print(voltage, 2); // Display voltage with 2 decimal places
  Serial.print(", Current (mA): ");
  Serial.println(current, 2); // Display current with 2 decimal places
  
  delay(1000); // Delay for stability
}


Voltages getAverageAnalogRead(int pin)
{
    constexpr size_t loops { 100 };
    constexpr float toV { 3.3f / float { (1 << adcResolution) - 1 } };
    constexpr float rDiv { 17.4f / ( 10.0f + 17.4f) };  //voltage divider from 5v at mux to 3v3 at ADC

    int tot { 0 };

    for (auto i = 0u; i < loops; i++)
        tot += Input.analogRead(pin);
    const auto avg = static_cast<float>(tot) * toV / static_cast<float>(loops);

    return { avg, avg / rDiv };
}