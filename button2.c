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
  STATUS_0,
  STATUS_1,
  STATUS_2,
  STATUS_3,
  STATUS_4,
  STATUS_LONG_PRESS
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
      longPressDetected = true;  //feed back to interupt
      //lcdUpdateNeeded = true; //feed to button action
      
      buttonStatus = STATUS_LONG_PRESS;
    }
  }

  switch (buttonStatus) {
      case STATUS_0:  // will execute always the button is not being pressed.
        Serial.print("Taps: ");
        Serial.println(taps);
        break;

      case STATUS_1:  // will execute when the button is pressed once.
        Serial.print("Taps: ");
        Serial.println(taps);
        break;

      case STATUS_2:  // will execute when the button is pressed twice.
        Serial.print("Taps: ");
        Serial.println(taps);
        break;

      case STATUS_3:  // will execute when the button is pressed three times.
        Serial.print("Taps: ");
        Serial.println(taps);
        break;

      case STATUS_4:  // will execute when the button is pressed four times.
        Serial.print("Taps: ");
        Serial.println(taps);
        break;
      case STATUS_LONG_PRESS:  // will execute when the button is pressed four times.
        Serial.println("Long Press Detected");
        break;

      default:
        Serial.println("Too Many Taps");
        buttonStatus = STATUS_0;
        break;
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

