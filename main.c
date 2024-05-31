/**
  Smart Irrigation System using the Arduino Edge Control - Application Note
  Name: Edge_Control_Code.ino
  Purpose: 4 Zones Smart Irrigation System using the Arduino Edge Control with Cloud connectivity using a MKR WiFi 1010.

  @author Christopher Mendez
*/

#include "Helpers.h"
#include <Arduino_EdgeControl.h>
#include <Wire.h>
#include "shared.hpp"

// The MKR1 board I2C address
#define EDGE_I2C_ADDR 0x05

/** UI Management **/
// Button statuses
enum ButtonStatus : byte {
  ZERO_TAP,
  SINGLE_TAP,
  DOUBLE_TAP,
  TRIPLE_TAP,
  QUAD_TAP,
  LOT_OF_TAPS
};

// ISR: count the button taps
volatile byte taps{ 0 };
// ISR: keep elapsed timings
volatile unsigned long previousPress{ 0 };
// ISR: Final button status
volatile ButtonStatus buttonStatus{ ZERO_TAP };

volatile bool lcdUpdateNeeded = false;

volatile unsigned long gallons = 0;
volatile unsigned long gallonTimer_prev = 0;
volatile unsigned long gallonTimer = 0;
volatile float gpm = 0;
volatile bool flow_ON = false;


///5V INPUT SETUP
constexpr unsigned int adcResolution { 12 };
constexpr size_t loops { 100 };  //number of readings to average 5V input 
constexpr float toV { 3.3f / float { (1 << adcResolution) - 1 } };  //ADC conversion to Volts
constexpr float rDiv { 17.4f / ( 10.0f + 17.4f) };  //Voltage divider between MUX and ADC
constexpr float CURRENT_LOOP_RESISTOR = 220.0f;  // 420ma current loop resisto

/////////////TANKS
constexpr float TANK_RADIUS_INCHES = 12.6f * 12.0f / 2.0f; //radius inches for 12.6' inside diamter tank
constexpr float TANK_CROSS_SECTION_AREA = 3.14f * TANK_RADIUS_INCHES * TANK_RADIUS_INCHES; // in^2
constexpr float PRESSURE_SENSOR_MAX = 15.0f;  // Maximum pressure sensor value in psi
constexpr float SENSOR_OFFSET_HEIGHT = 0.0f;  // Sensor offset from tank bottom in inches
constexpr float PSI_TO_HEIGHT_CONVERSION = 2.31f * 12.0f;  // Conversion factor from psi to inches of water
constexpr float INCHES_CUBIC_TO_GALLONS = 231.0f;  // Conversion factor from cubic inches to gallons
///////////

unsigned long previousMillis = 0;  // will store last time the sensors were updated

const long interval = 1000;  // interval at which to update sensores (milliseconds)

// Valves flow control variables
bool controlV1 = 1;  //actual status
bool controlV2 = 1;
bool controlV3 = 1;
bool controlV4 = 1;

// LCD flow control variables
bool controlLCD = 1;
int showTimeLCD = 0;

// Valves On time kepping variables
int StartTime1, CurrentTime1;
int StartTime2, CurrentTime2;
int StartTime3, CurrentTime3;
int StartTime4, CurrentTime4;

SensorValues_t vals;



/**
  Main section setup  
  TBD add valve initialization to close all valves on startup
*/
void setup() {
  EdgeControl.begin();
  Wire.begin();
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
    Serial.println("Please, be sure to enable gated 3V3 and 5V power rails");
    Serial.println("via Power.enable3V3() and Power.enable5V().");
  } else Serial.println("succeeded.");

  // Arduino Edge Control ports init
  Input.begin();
  Input.enable();
  Latching.begin();

  analogReadResolution(adcResolution);

  //IRQ setups
  pinMode(IRQ_CH1, INPUT); // Flow meter definition
  attachInterrupt(digitalPinToInterrupt(IRQ_CH1), gallonCounter, RISING);

  pinMode(POWER_ON, INPUT); // LCD button definition
  attachInterrupt(POWER_ON, buttonPress, RISING);

  //clock
  setSystemClock(__DATE__, __TIME__);  // define system time as a reference for the RTC

  //RealTimeClock.setEpoch(1684803771-(3600*4));  // uncomment this to set the RTC time once.

  // Init the LCD display
  LCD.begin(16, 2);
  LCD.backlight();

  LCD.home();
  LCD.print("Smart Irrigation");
  LCD.setCursor(5, 1);
  LCD.print("System");
  delay(2000);

  LCD.clear();
}

void loop() {
  // LCD button taps detector function
  if (lcdUpdateNeeded) {
    LCD.clear();
    lcdUpdateNeeded = false;
  }
  Serial.println(gallons);
  //detectTaps();
  //t1++;
  //t2--;

  // Different button taps handler
  switch (buttonStatus) {
    case ZERO_TAP:  // will execute always the button is not being pressed.
      timeLCD();
      break;

    case SINGLE_TAP:  // will execute when the button is pressed once.
      tankStatusLCD();
      break;

    case DOUBLE_TAP:  // will execute when the button is pressed twice.
      flowStatusLCD();
      break;

    case TRIPLE_TAP:  // will execute when the button is pressed three times.
      ValvesStatusLCD();
      break;

    case QUAD_TAP:  // will execute when the button is pressed four times.
      ValvesStatus2LCD();
      break;

    

    default:
      Serial.println("Too Many Taps");
      buttonStatus = ZERO_TAP;
      break;
  }

  // reset the valves accumuldated on time every day at midnight
  if (getLocalhour() == " 00:00:00") {
    Serial.println("Resetting accumulators every day");
    vals.z1_on_time = 0;
    vals.z2_on_time = 0;
    vals.z3_on_time = 0;
    vals.z4_on_time = 0;
    delay(1000);
  }

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {

    previousMillis = currentMillis;

    // send local sensors values and retrieve cloud variables status back and forth
    updateSensors();
  }

  // activate, deactive and keep time of valves function
  valvesHandler();
}

/**
  Poor-man LCD button debouncing function. 
*/
void buttonPress() {
  const auto now = millis();

  if (now - previousPress > 100) {
    previousPress = now;
    taps++;
    if (taps > 4) {
      taps = 0;
    }
    lcdUpdateNeeded = true;
  }
  buttonStatus = static_cast<ButtonStatus>(taps);
}

/**
  Detect and count button taps
*/
void detectTaps() {
  // Timeout to validate the button taps counter
  //constexpr unsigned int buttonTapsTimeout{ 300 };

  // Set the button status and reset the taps counter when button has been
  // pressed at least once and button taps validation timeout has been reached.
  //if (taps > 0 && millis() - previousPress >= buttonTapsTimeout) {
  buttonStatus = static_cast<ButtonStatus>(taps);
  
  
}

/**
  This function retrieve data from the MKR to update the local valves status, also reads the water level sensor
  and send back the valves status if there was any manual change to the MKR to be updated in the cloud.
*/
void updateSensors() {

  // function that request valves status from the MKR through I2C
  //getCloudValues();

  
  vals.tankLevel_1 = getTankVolume(INPUT_420mA_CH01);
  vals.tankLevel_2 = getTankVolume(INPUT_420mA_CH02);
  
  //vals.gallons = gallons;
  //vals.GPM = gpm;

  //calculate GPM


  //vals.z1_on_time;  //doesn't do anything
  //vals.z2_on_time;
  //vals.z3_on_time;
  //vals.z4_on_time;

  //sendValues(&vals);

  // Uncomment this function to read in the serial monitor the values received by the I2C communication
  //logValues(&vals);
}

/**
    Debugging function to print the variables status being sent and read from the MKR

    @param values The I2C communicated sensors values
  */
  void logValues(SensorValues_t *values) {
    char telem_buf[200];
    sprintf(telem_buf, "Zone 1: %d, Zone 2: %d, Zone 3: %d, Zone 4: %d, Timer 1: %d,Timer 2: %d,Timer 3: %d,Timer 4: %d, Water Level: %.2f",
            values->z1_local, values->z2_local, values->z3_local, values->z4_local, values->z1_count, values->z2_count, values->z3_count, values->z4_count, values->water_level_local);

    Serial.println(telem_buf);
  }

/**
  Function to request a valves and variables update from the MKR.
*/

void getCloudValues() {

  Wire.beginTransmission(EDGE_I2C_ADDR);  //slave's 7-bit I2C address

  // asking if the MKR board is connected to be able to communicate
  byte busStatus = Wire.endTransmission();

  if (busStatus != 0) {  // if the slave is not connected this happens

  } else {  // if the slave is connected this happens

    Wire.requestFrom(EDGE_I2C_ADDR, sizeof(SensorValues_t));

    uint8_t buf[200];
    for (uint8_t i = 0; i < sizeof(SensorValues_t); i++)
      if (Wire.available())
        buf[i] = Wire.read();

    SensorValues_t *values = (SensorValues_t *)buf;

    vals.z1_local = values->z1_local;
    vals.z2_local = values->z2_local;
    vals.z3_local = values->z3_local;
    vals.z4_local = values->z4_local;

    if (values->z1_count > 0 || values->z2_count > 0 || values->z3_count > 0 || values->z4_count > 0) {
      Serial.println("There's an active timer");
      showTimeLCD = 1;  // instead of showing valves status in the LCD, shows the countdown timers.
    } else {
      Serial.println("There's not an active timer");
      showTimeLCD = 0;  // show the valves satus in the LCD.
    }

    // convert seconds to minutes (these variables store the countdown timers)
    vals.z1_count = values->z1_count / 60.0;
    vals.z2_count = values->z2_count / 60.0;
    vals.z3_count = values->z3_count / 60.0;
    vals.z4_count = values->z4_count / 60.0;
  }
}

/**
  Function that sends the local sensors values through I2C to the MKR
  @param values The I2C communicated sensors values
*/
void sendValues(SensorValues_t *values) {
  writeBytes((uint8_t *)values, sizeof(SensorValues_t));
}

/**
  Function that transport the sensors data through I2C to the MKR
  @param buf store the structured sensors values
  @param len store the buffer lenght
*/
void writeBytes(uint8_t *buf, uint8_t len) {

  Wire.beginTransmission(EDGE_I2C_ADDR);

  for (uint8_t i = 0; i < len; i++) {
    Wire.write(buf[i]);
  }

  Wire.endTransmission();
}

/**
  Function that reads the valves values received from the MKR and controls them,
  measures the ON time of each one and update their status on the LCD screen.
*/
void valvesHandler() {

  if (vals.z1_local == 1 && controlV1 == 1) {
    Serial.println("Opening Valve 1");
    LCD.clear();
    LCD.setCursor(1, 0);
    LCD.print("Opening Valve");
    LCD.setCursor(7, 1);
    LCD.print("#1");
    controlLCD = 1;
    Latching.channelDirection(LATCHING_OUT_1, NEGATIVE);
    Latching.strobe(4500);
    StartTime1 = time(NULL);
    controlV1 = 0;
    LCD.clear();
  } else if (vals.z1_local == 0 && controlV1 == 0) {
    Serial.println("Closing Valve 1");
    LCD.clear();
    LCD.setCursor(1, 0);
    LCD.print("Closing Valve");
    LCD.setCursor(7, 1);
    LCD.print("#1");
    controlLCD = 1;
    Latching.channelDirection(LATCHING_OUT_1, POSITIVE);
    Latching.strobe(4500);
    CurrentTime1 = time(NULL);
    Serial.print("V1 On Time: ");
    vals.z1_on_time += (CurrentTime1 - StartTime1) / 60.0;
    Serial.println(vals.z1_on_time);
    controlV1 = 1;
    LCD.clear();
  }

  if (vals.z2_local == 1 && controlV2 == 1) {
    Serial.println("Opening Valve 2");
    LCD.clear();
    LCD.setCursor(1, 0);
    LCD.print("Opening Valve");
    LCD.setCursor(7, 1);
    LCD.print("#2");
    controlLCD = 1;
    Latching.channelDirection(LATCHING_OUT_3, NEGATIVE);
    Latching.strobe(4500);
    StartTime2 = time(NULL);
    controlV2 = 0;
    LCD.clear();
  } else if (vals.z2_local == 0 && controlV2 == 0) {
    Serial.println("Closing Valve 2");
    LCD.clear();
    LCD.setCursor(1, 0);
    LCD.print("Closing Valve");
    LCD.setCursor(7, 1);
    LCD.print("#2");
    controlLCD = 1;
    Latching.channelDirection(LATCHING_OUT_3, POSITIVE);
    Latching.strobe(4500);
    CurrentTime2 = time(NULL);
    Serial.print("V2 On Time: ");
    vals.z2_on_time += (CurrentTime2 - StartTime2) / 60.0;
    Serial.println(vals.z2_on_time);
    controlV2 = 1;
    LCD.clear();
  }

  if (vals.z3_local == 1 && controlV3 == 1) {
    Serial.println("Opening Valve 3");
    LCD.clear();
    LCD.setCursor(1, 0);
    LCD.print("Opening Valve");
    LCD.setCursor(7, 1);
    LCD.print("#3");
    controlLCD = 1;
    Latching.channelDirection(LATCHING_OUT_5, NEGATIVE);
    Latching.strobe(4500);
    StartTime3 = time(NULL);
    controlV3 = 0;
    LCD.clear();
  } else if (vals.z3_local == 0 && controlV3 == 0) {
    Serial.println("Closing Valve 3");
    LCD.clear();
    LCD.setCursor(1, 0);
    LCD.print("Closing Valve");
    LCD.setCursor(7, 1);
    LCD.print("#3");
    controlLCD = 1;
    Latching.channelDirection(LATCHING_OUT_5, POSITIVE);
    Latching.strobe(4500);
    CurrentTime3 = time(NULL);
    Serial.print("V3 On Time: ");
    vals.z3_on_time += (CurrentTime3 - StartTime3) / 60.0;
    Serial.println(vals.z3_on_time);
    controlV3 = 1;
    LCD.clear();
  }

  if (vals.z4_local == 1 && controlV4 == 1) {
    Serial.println("Opening Valve 4");
    LCD.clear();
    LCD.setCursor(1, 0);
    LCD.print("Opening Valve");
    LCD.setCursor(7, 1);
    LCD.print("#4");
    controlLCD = 1;
    Latching.channelDirection(LATCHING_OUT_7, NEGATIVE);
    Latching.strobe(4500);
    StartTime4 = time(NULL);
    controlV4 = 0;
    LCD.clear();
  } else if (vals.z4_local == 0 && controlV4 == 0) {
    Serial.println("Closing Valve 4");
    LCD.clear();
    LCD.setCursor(1, 0);
    LCD.print("Closing Valve");
    LCD.setCursor(7, 1);
    LCD.print("#4");
    controlLCD = 1;
    Latching.channelDirection(LATCHING_OUT_7, POSITIVE);
    Latching.strobe(4500);
    CurrentTime4 = time(NULL);
    Serial.print("V4 On Time: ");
    vals.z4_on_time += (CurrentTime4 - StartTime4) / 60.0;
    Serial.println(vals.z4_on_time);
    controlV4 = 1;
    LCD.clear();
  }
}



/**
  Function that shows each valve current status on the LCD screen.
*/
void ValvesStatusLCD() {
  String msg, msg2;

  msg = "Main: ";
  msg += "OPEN";
  //msg += " 2:";
  //msg += "CLOSE";
  msg2 = "Dev5: ";
  msg2 += "CLOSED";
  //msg2 += "  4:";
 // msg2 += vals.z4_local;
/*
  msg = "Main:";
  msg += vals.z1_local;
  msg += "    Dev5:";
  msg += vals.z2_local;
  msg2 = "LTerr:";
  msg2 += vals.z3_local;
  msg2 += "  LCont:";
  msg2 += vals.z4_local;
*/
  //LCD.clear();
  LCD.setCursor(0, 0);
  //LCD.print("Valves: ");
  LCD.print(msg);
  LCD.setCursor(0, 1);
  LCD.print(msg2);
}

void ValvesStatus2LCD() {
  String msg, msg2;

  msg = "L Cont: ";
  msg += "CLOSED";
  //msg += " 2:";
  //msg += "CLOSE";
  msg2 = "L Terr: ";
  msg2 += "CLOSED";
  //msg2 += "  4:";
 // msg2 += vals.z4_local;
/*
  msg = "Main:";
  msg += vals.z1_local;
  msg += "    Dev5:";
  msg += vals.z2_local;
  msg2 = "LTerr:";
  msg2 += vals.z3_local;
  msg2 += "  LCont:";
  msg2 += vals.z4_local;
*/
  //LCD.clear();
  LCD.setCursor(0, 0);
  //LCD.print("Valves: ");
  LCD.print(msg);
  LCD.setCursor(0, 1);
  LCD.print(msg2);
}


/**
  Function that shows Tank Vol and Flow on the LCD screen.
*/
void tankStatusLCD() {

  char line1[16];
  char line2[16];
  
  sprintf(line1, "Tank1: %5.0f gal", vals.tankLevel_1);  // use sprintf() to compose the string line1
  sprintf(line2, "Tank2: %5.0f gal", vals.tankLevel_2);  // use sprintf() to compose the string line2
  
  LCD.setCursor(0, 0);
  LCD.print(line1);
  LCD.setCursor(0, 1);
  LCD.print(line2);

}

void timeLCD() {

  LCD.setCursor(0, 0);
  LCD.print("Hossfeld");

  LCD.setCursor(11, 0);
  LCD.print(getLocalhour());

  LCD.setCursor(0, 1);
  LCD.print("Vineyards");

  LCD.setCursor(11, 1);
  LCD.print(getDay());
}

void flowStatusLCD() {
  char line1[16];
  char line2[16];
  
  sprintf(line1, "today: %5d gal", gallons);  // use sprintf() to compose the string line2
  sprintf(line2, "Flow:  %5.0f GPM", gpm);  // use sprintf() to compose the string line1
  
  LCD.setCursor(0, 0);
  LCD.print(line1);
  LCD.setCursor(0, 1);
  LCD.print(line2);

}


float getTankVolume(int pin) {
    float sensorValue = getAverage5vRead(pin);
    float current_mA = (sensorValue * 1000.0f / CURRENT_LOOP_RESISTOR);  // Convert sensor reading to mA 
    float pressure = (current_mA - 4.0f) / 16.0f * PRESSURE_SENSOR_MAX;  // Convert mA to psi
    float waterHeight = pressure * PSI_TO_HEIGHT_CONVERSION;  // Convert psi to inches of water column
    waterHeight -= SENSOR_OFFSET_HEIGHT;  // Adjust for sensor location below tank bottom
    float volumeInCubicInches = waterHeight * TANK_CROSS_SECTION_AREA;  // Volume in cubic inches
    float volumeInGallons = volumeInCubicInches / INCHES_CUBIC_TO_GALLONS;  // Convert cubic inches to gallons
    
    return volumeInGallons;
}

///////////

float getFlow(int pin){ //TBD
    float flow = getAverage5vRead(pin);


    return flow;
}

float getAverage5vRead(int pin){

    int tot { 0 };
    for (auto i = 0u; i < loops; i++)
        tot += Input.analogRead(pin);
    const auto avg5v = static_cast<float>(tot) * toV / static_cast<float>(loops) / rDiv;
    
    return (avg5v);
}

// Function to be called on interrupt
  
void gallonCounter() {
  const auto now = millis();

  if (now - previousPress > 100) {
    previousPress = now;
    gallons++;
    gallonTimer_prev = gallonTimer;
    gallonTimer = millis();

    if (flow_ON) {
        
      gpm = 60000.0f / (gallonTimer - gallonTimer_prev);  //each ISR is 1 gallon.  convert elasped time to minutes
        
    } else {
        gpm = 0;
        flow_ON = true;
    }
  }
}