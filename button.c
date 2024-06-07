#include <Arduino.h>
#include <Arduino_EdgeControl.h>

// Pin definition
const int buttonPin = POWER_ON;



// Variables for button press tracking
volatile unsigned long lastPressTime = 0;
volatile unsigned long buttonDownTime = 0;
volatile int taps = 0;
volatile bool lcdUpdateNeeded = false;
volatile bool longPressDetected = false;
volatile bool buttonPressed = false;

enum ButtonStatus { 
  BUTTON_STATUS_0,
  BUTTON_STATUS_1,
  BUTTON_STATUS_2,
  BUTTON_STATUS_3,
  BUTTON_STATUS_4,
  BUTTON_STATUS_LONG_PRESS
};

volatile ButtonStatus buttonStatus;

void setup() {
  pinMode(buttonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonPress, CHANGE); // Use CHANGE to detect both press and release
  Serial.begin(9600);
  // Initialize your LCD and other setup code here
}

void loop() {
  if (buttonPressed && !longPressDetected) {
    unsigned long now = millis();
    if (now - buttonDownTime >= 2000) { // Long press check
      longPressDetected = true;
      lcdUpdateNeeded = true;
      buttonStatus = BUTTON_STATUS_LONG_PRESS;
    }
  }

  if (lcdUpdateNeeded) {
    noInterrupts(); // Temporarily disable interrupts
    bool updateNeeded = lcdUpdateNeeded;
    lcdUpdateNeeded = false;
    int currentTaps = taps;
    bool longPress = longPressDetected;
    longPressDetected = false;
    interrupts(); // Re-enable interrupts

    if (updateNeeded) {
      updateLCD(currentTaps, longPress); // Implement your LCD update logic here
    }
  }
}

void updateLCD(int taps, bool longPress) {
  // Implement your LCD update logic based on the number of taps
  if (longPress) {
    // Handle long press logic
    Serial.println("Long Press Detected");
  } else {
    // Handle tap logic
    Serial.print("Taps: ");
    Serial.println(taps);
  }
}

void buttonPress() {
  unsigned long now = millis();
  if (digitalRead(buttonPin) == LOW) { // Button is pressed
    if (!buttonPressed) { // First press
      buttonPressed = true;
      buttonDownTime = now;
    }
  } else { // Button is released
    if (buttonPressed) { // Was previously pressed
      buttonPressed = false;
      if (!longPressDetected && (now - buttonDownTime > 100)) { // Debounce check and not a long press
        taps++;
        if (taps > 4) {
          taps = 0;
        }
        lcdUpdateNeeded = true;
        buttonStatus = static_cast<ButtonStatus>(taps);
      }
      longPressDetected = false; // Reset long press detection
    }
  }
}

