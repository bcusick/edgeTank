
#include <Arduino_EdgeControl.h>

volatile bool buttonPressed = false;

unsigned long lastPress {0};
const int debounce {200};

////Valve Setup
const int pulseDuration {10000}; //milliseconds 

typedef struct {
  bool status   = false; //false is closed
  bool cmd    = false;
  int fault = 0; 
  bool autoControl = true;
} Valves_t;

Valves_t mainValve;

void setup() {
  EdgeControl.begin();
  
  delay(500);
  Serial.begin(115200);

  Serial.println("Init begin");

  // Enable power lines
  Power.enable3V3();
  Power.enable5V();
  Power.on(PWR_3V3);
  Power.on(PWR_VBAT);
  Power.on(PWR_MKR1);
  delay(5000);
  Power.on(PWR_19V);

  // Init Edge Control IO Expander
  Serial.print("IO Expander initializazion ");
  if (!Expander.begin()) {
    Serial.println("failed.");
  } else Serial.println("succeeded.");
  
  // Arduino Edge Control ports init
  Input.begin();
  Input.enable();
  Latching.begin();
  //analogReference(V_REF); //2.048V
  //analogReference(AR_VDD); //3.3V
  //analogReadResolution(adcResolution);
  
  //IRQ setups
  pinMode(POWER_ON, INPUT_PULLUP); // LCD button definition
  attachInterrupt(digitalPinToInterrupt(POWER_ON), buttonPress, RISING);

  // Init the LCD display
  LCD.begin(16, 2);
  LCD.backlight();

  LCD.home();
  LCD.print("Valve Test");
}

void loop() {

  LCD.home();
  LCD.print("Valve Test");
  if (buttonPressed) {
    mainValve.cmd = !mainValve.cmd;
    buttonPressed = false;
  }
  valvesHandler();  //execute open/close and set control flag

}

void buttonPress() {
  unsigned long now = millis();
  if ((now - lastPress) > debounce) {
    buttonPressed = true;
    lastPress = now;
  }
}

void valvesHandler() {
  if (mainValve.cmd == 1 && mainValve.status == 0) {
    Serial.println("Opening Main");
    LCD.clear();
    LCD.setCursor(1, 0);
    LCD.print("Opening Main");
    Latching.channelDirection(LATCHING_OUT_1, POSITIVE);
    Latching.strobe(pulseDuration);
    mainValve.status = 1;  //set status to open
    LCD.clear();
  } else if (mainValve.cmd == 0 && mainValve.status == 1) {
    Serial.println("Closing Main");
    LCD.clear();
    LCD.setCursor(1, 0);
    LCD.print("Closing Main");
    Latching.channelDirection(LATCHING_OUT_1, NEGATIVE);
    Latching.strobe(pulseDuration);
    mainValve.status = 0;  //set status to closed
    LCD.clear();
  }
}

