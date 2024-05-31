// Constants
const byte flowMeterPin = 2;  // Pin where the flowmeter is connected
const float minFlowRate = 0.5; // Minimum flow rate in feet per second
const float maxFlowRate = 30.0; // Maximum flow rate in feet per second
const float minFrequency = 3.2; // Minimum frequency in Hz
const float maxFrequency = 200.0; // Maximum frequency in Hz


constexpr float FPS_TO_GPM = 12.0f * 3.14f * 1.0f * 1.0f / 231.0f * 60.0f;  //2inch pipe




// Variables
volatile unsigned long pulseCount = 0; // Pulse count from the flowmeter
unsigned long lastMillis = 0; // To store last time in milliseconds

// Function to be called on interrupt
void pulseCounter() {
  pulseCount++;
}

void setup() {
  // Initialize serial communication
  Serial.begin(9600);

  // Set flow meter pin as input
  pinMode(flowMeterPin, INPUT);

  // Attach interrupt to the flow meter pin
  attachInterrupt(digitalPinToInterrupt(flowMeterPin), pulseCounter, RISING);

  // Initialize lastMillis
  lastMillis = millis();

  /////////////my code

  // Attach callbacks to IRQ pins
    attachInterrupt(digitalPinToInterrupt(IRQ_CH1), pulseCounter, RISING);  //IRQ_CH1 attached to Main Flowmeter
}

void loop() {
  unsigned long currentMillis = millis();

  // Update flow rate every second
  if (currentMillis - lastMillis >= 1000) {
    // Calculate frequency
    float frequency = pulseCount / time_to_last_count(millis) * 1000f; // pulses per second  //TBD implement timer to last count

    // Map frequency to flow rate
    float flowRate = map(frequency, minFrequency, maxFrequency, minFlowRate, maxFlowRate);
    flowRate *= FPS_TO_GPM;

    

    // Reset pulse count and update lastMillis
    pulseCount = 0;
    lastMillis = currentMillis;
  }
}

float map(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

