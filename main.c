
#include "Helpers.h"
#include <Arduino_EdgeControl.h>
#include <Wire.h>
#include "shared.hpp"
#include <SPI.h>
#include <SD.h>

// The MKR1 board I2C address
#define EDGE_I2C_ADDR 0x05

/** UI Management **/
// Button statuses
enum ButtonStatus {
  STATUS_0,
  STATUS_1,
  STATUS_2,
  STATUS_3,
  STATUS_4
};

// ISR variables
volatile byte taps{ 0 };
volatile ButtonStatus buttonStatus{ STATUS_0 };
volatile bool lcdUpdateNeeded = false;
volatile unsigned long buttonDownTime = 0;
volatile bool longPressDetected = false;
volatile bool buttonPressed = false;

////Datalogging
String logName = "test1.txt";
const int chipSelect = PIN_SD_CS;
File dataFile;

////5V INPUT SETUP
constexpr unsigned int adcResolution { 12 };
constexpr size_t loops { 10 };  //number of readings to average 5V input 
constexpr float toV { 3.3f / float { (1 << adcResolution) - 1 } };  //ADC conversion to Volts
constexpr float rDiv { 17.4f / ( 10.0f + 17.4f) };  //Voltage divider between MUX and ADC
constexpr float calibration5v { 1.017f };  //Voltage divider between MUX and ADC
constexpr float CURRENT_LOOP_RESISTOR { 220.5f };  // 420ma current loop resisto

////TANKS
constexpr float TANK_RADIUS_INCHES {12.6f * 12.0f / 2.0f}; //radius inches for 12.6' inside diamter tank
constexpr float TANK_CROSS_SECTION_AREA {3.14f * TANK_RADIUS_INCHES * TANK_RADIUS_INCHES}; // in^2
constexpr float PRESSURE_SENSOR_MAX {15.0f};  // Maximum pressure sensor value in psi
constexpr float SENSOR_OFFSET_HEIGHT {0.0f};  // Sensor offset from tank bottom in inches
constexpr float PSI_TO_HEIGHT_CONVERSION {2.31f * 12.0f};  // Conversion factor from psi to inches of water
constexpr float INCHES_CUBIC_TO_GALLONS {231.0f};  // Conversion factor from cubic inches to gallons
///////////

////Flow Sensor
constexpr float FLOW_SENSOR_MAX {53.0f};  // Maximum flow sensor value in GPM.  Can be set on sensor.

unsigned long previousMillis_loop1 {0};  // will store last time the sensors were updated
unsigned long previousMillis_loop2 {0};  // will store last time the sensors were updated
const long interval_loop1 {1000};  // interval at which to update sensores (milliseconds)
const long interval_loop2 {60000};  // interval at which to update log  (milliseconds)

////Battery
float VBat {0};
const float calBat {1.0455};

////Valve Setup
const int pulseDuration {12000}; //milliseconds 
const int maxRetries {5};
int valveRetries {0}; //check FlowSensor against valve status.  throw error after #of retries

////Analog rolling average 
int readIndex {0};  // The index of the current reading
const int numReadings {60};  // Number of readings to store for rolling averages

////Schedule
int currentHour {0};

////Define data structures
SensorValues_t vals;
TimeKeeping_t timing;
TankValues_t tank1;
TankValues_t tank2;
Valves_t mainValve;

// Valves On time kepping variables
//int StartTime1, CurrentTime1;
//int StartTime2, CurrentTime2;
//int StartTime3, CurrentTime3;
//int StartTime4, CurrentTime4;



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
  } else Serial.println("succeeded.");

  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialized.");

  dataFile = SD.open(logName, FILE_WRITE);
  
  // If the file opened successfully, write the header
  if (dataFile) {
    dataFile.println("Time,Tank1,Tank2,GPM,Gallons,Battery");
    dataFile.close();
  } else {
    Serial.println("Error opening log file");
  }

  // Arduino Edge Control ports init
  Input.begin();
  Input.enable();
  Latching.begin();
  //analogReference(V_REF); //2.048V
  //analogReference(AR_VDD); //3.3V
  analogReadResolution(adcResolution);
  
  //IRQ setups
  pinMode(POWER_ON, INPUT_PULLUP); // LCD button definition
  attachInterrupt(digitalPinToInterrupt(POWER_ON), buttonPress, CHANGE);

  //clock
  setSystemClock(__DATE__, __TIME__);  // define system time as a reference for the RTC

  // Init the LCD display
  LCD.begin(16, 2);
  LCD.backlight();

  LCD.home();
  LCD.print("Smart Irrigation");
  LCD.setCursor(5, 1);
  LCD.print("System");
  delay(2000);

  LCD.clear();

  for (int i = 0; i < numReadings; i++) {
    tank1.readings[i] = 0;
    tank2.readings[i] = 0;
  }
}

void loop() {

  currentHour = getHour();
  
  //scheduling 
  switch (currentHour){
    case 0:
      if (!vals.isReset) {
        vals.gallons = 0;
        vals.isReset = true;
      }
      if (mainValve.autoControl) {  //auto control
        mainValve.cmd = 0;
      }
      break;
    case 1:
      if (mainValve.autoControl) {  //auto control
        mainValve.cmd = 0;
      }
      break;
    case 2:
      if (mainValve.autoControl) {  //auto control
        mainValve.cmd = 0;
      }
      break;
    case 3:
      if (mainValve.autoControl) {  //auto control
        mainValve.cmd = 0;
      }
      break;
    case 4:
      if (mainValve.autoControl) {  //auto control
        mainValve.cmd = 0;
      }
      break;
    case 5:
      if (mainValve.autoControl) {  //auto control
        mainValve.cmd = 0;
      }
      break;
    case 6:
      if (mainValve.autoControl) {  //auto control
        mainValve.cmd = 1;
        //mainValve.fault = 0;
      }
      break;
    case 7:
      if (mainValve.autoControl) {  //auto control
        mainValve.cmd = 1;
      }
      break;
    case 8:
      if (mainValve.autoControl) {  //auto control
        mainValve.cmd = 1;
      }
      break;
    case 9:
      if (mainValve.autoControl) {  //auto control
        mainValve.cmd = 1;
      }
      break;
    case 10:
      if (mainValve.autoControl) {  //auto control
        mainValve.cmd = 1;
      }
      break;
    case 11:
      if (mainValve.autoControl) {  //auto control
        mainValve.cmd = 1;
      }
      break;
    case 12:
      if (mainValve.autoControl) {  //auto control
        mainValve.cmd = 1;
      }
      break;
    case 13:
      if (mainValve.autoControl) {  //auto control
        mainValve.cmd = 1;
      }
      break;
    case 14:
      if (mainValve.autoControl) {  //auto control
        mainValve.cmd = 1;
      }
      break;
    case 15:
      if (mainValve.autoControl) {  //auto control
        mainValve.cmd = 1;
      }
      break;
    case 16:
      if (mainValve.autoControl) {  //auto control
        mainValve.cmd = 1;
      }
      break;
    case 17:
      if (mainValve.autoControl) {  //auto control
        mainValve.cmd = 1;
      }
      break;
    case 18:
      if (mainValve.autoControl) {  //auto control
        mainValve.cmd = 1;
      }
      break;
    case 19:
      if (mainValve.autoControl) {  //auto control
        mainValve.cmd = 0;
      }
      break;
    case 20:
      if (mainValve.autoControl) {  //auto control
        mainValve.cmd = 0;
      }
      break;
    case 21:
      if (mainValve.autoControl) {  //auto control
        mainValve.cmd = 0;
      }
      break;
    case 22:
      if (mainValve.autoControl) {  //auto control
        mainValve.cmd = 0;
      }
      break;
    case 23:
      if (mainValve.autoControl) {  //auto control
        mainValve.cmd = 0;
      }
      vals.isReset = false; //prepare for counter reset @ midnight 
      break;
  }

  if (buttonPressed && !longPressDetected) {
    unsigned long now = millis();
    if (now - buttonDownTime >= 2000) { // Long press check
      longPressDetected = true;  //feed back to interupt
         
      if (buttonStatus == STATUS_0) {  //Long Press on Home Screen
        if (mainValve.autoControl){
          mainValve.autoControl = false;
          LCD.clear();
          LCD.setCursor(0, 0);
          LCD.print("Manual Mode");
          delay(1000);
          LCD.clear();
        }
        else if (!mainValve.autoControl){
          mainValve.autoControl = true;
          LCD.clear();
          LCD.setCursor(0, 0);
          LCD.print("Auto Mode");
          delay(1000);
          LCD.clear();
        }
      }
      if (buttonStatus == STATUS_3 && mainValve.autoControl == false) {  //Long Press on Main Valve Screen
        mainValve.cmd = !mainValve.cmd; //request opposite cmd
        mainValve.fault = 0; //reset feedback 
      }
    }
  }
  
  unsigned long currentMillis = millis();
///main update interval
  if (currentMillis - previousMillis_loop1 >= interval_loop1) {
    updateSensors();
    updateGallons();
    
    valvesHandler();  //execute open/close and set control flag
    
    if (lcdUpdateNeeded) {
    LCD.clear();
    lcdUpdateNeeded = false;
    }

    // Different button taps handler
    switch (buttonStatus) {
      case STATUS_0:  
        timeLCD();
        break;

      case STATUS_1:  
        tankStatusLCD();
        break;

      case STATUS_2:  
        flowStatusLCD();
        break;

      case STATUS_3:  
        ValvesStatusLCD();
        break;

      default:
        Serial.println("Too Many Taps");
        buttonStatus = STATUS_0;
        break;
    }
    previousMillis_loop1 = currentMillis;
  }

  ///slower loop to log data 
  if (currentMillis - previousMillis_loop2 >= interval_loop2) {
    checkValveStatus(); 
    VBat = Power.getVBat(adcResolution) / calBat;

    //convert to int values before datalog and send to cloud?
    logDataToSD();
    previousMillis_loop2 = currentMillis;
  }
}

/**
  LCD button debouncing function. 
  Attached to interupt
*/
void buttonPress() {
  unsigned long now = millis();
  if (digitalRead(POWER_ON) == LOW) { // Button is pressed
    if (!buttonPressed) { // First press
      buttonPressed = true;
      buttonDownTime = now;
    }
  } else { // Button is released
    if (buttonPressed) { // Was previously pressed
      buttonPressed = false;
      if (!longPressDetected && (now - buttonDownTime > 100)) { // Debounce check and not a long press
        taps++;
        if (taps > 3) {
          taps = 0;
        }
        buttonStatus = static_cast<ButtonStatus>(taps);
      }
      longPressDetected = false; // Reset long press detection
    }
    lcdUpdateNeeded = true;
  }
}

/**
  This function retrieve data from the MKR to update the local valves status, also reads the water level sensor
  and send back the valves status if there was any manual change to the MKR to be updated in the cloud.
*/
void updateSensors() {

  //tank1
  tank1.sensorTotal -= tank1.readings[readIndex];  //remove oldest value
  tank1.readings[readIndex] = getTankVolume(INPUT_420mA_CH01);
  tank1.sensorTotal += tank1.readings[readIndex];
  tank1.level = tank1.sensorTotal / (float)numReadings;
  
  //tank2
  tank2.sensorTotal -= tank2.readings[readIndex];  //remove oldest value
  tank2.readings[readIndex] = getTankVolume(INPUT_420mA_CH02);
  tank2.sensorTotal += tank2.readings[readIndex];
  tank2.level = tank2.sensorTotal / (float)numReadings;

  //flowmeter
  vals.gpm = getFlow(INPUT_420mA_CH03);

  readIndex = (readIndex + 1) % numReadings;

}

/**
    Debugging function to print the variables status being sent and read from the MKR

    @param values The I2C communicated sensors values
  */
  void logValues(SensorValues_t *values) {
    char telem_buf[200];
    //sprintf(telem_buf, "Zone 1: %d, Zone 2: %d, Zone 3: %d, Zone 4: %d, Timer 1: %d,Timer 2: %d,Timer 3: %d,Timer 4: %d, Water Level: %.2f",
            //values->z1_local, values->z2_local, values->z3_local, values->z4_local, values->z1_count, values->z2_count, values->z3_count, values->z4_count, values->water_level_local);

    Serial.println(telem_buf);
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

void timeLCD() {

  LCD.setCursor(0, 0);
  LCD.print("Hossfeld");

  LCD.setCursor(11, 0);
  LCD.print(getLocalhour());

  LCD.setCursor(0, 1);
  LCD.print("Vineyards");

  LCD.setCursor(13, 1);
  LCD.print(getDay());
}

void tankStatusLCD() {
  
  char line1[17]; // Extra space for the null terminator
  char line2[17]; // Extra space for the null terminator

  snprintf(line1, sizeof(line1), "Tank1: %5d gal", (int)tank1.level);
  snprintf(line2, sizeof(line2), "Tank2: %5d gal", (int)tank2.level);

  LCD.setCursor(0, 0);
  LCD.print(line1);
  LCD.setCursor(0, 1);
  LCD.print(line2);
}

void flowStatusLCD() {
  
  char line1[17]; // Extra space for the null terminator
  char line2[17]; // Extra space for the null terminator

  snprintf(line1, sizeof(line2), "Flow:  %5d GPM", (int)vals.gpm);
  snprintf(line2, sizeof(line1), "Today: %5d gal", (int)vals.gallons);
  
  LCD.setCursor(0, 0);
  LCD.print(line1);
  LCD.setCursor(0, 1);
  LCD.print(line2);
}

/**
  Function that shows each valve current status on the LCD screen.
*/
void ValvesStatusLCD() {
  String msg, msg2;

  msg = "Main: ";
  if (mainValve.fault == 0){
    msg += "waiting...";
  }
  else if (mainValve.cmd == 1 && mainValve.fault == 1){
    msg += "OPEN";
  }
  else if (mainValve.cmd == 0 && mainValve.fault == 1){
    msg += "CLOSED";
  }
  else if (mainValve.fault == 2){
    msg += "ERROR";
  }
  
  if (mainValve.autoControl) {
    msg2 = "Auto Control";
  }else if (!mainValve.autoControl) {
    msg2 = "Manual Control";
  }
  LCD.setCursor(0, 0);
  LCD.print(msg);
  LCD.setCursor(0, 1);
  LCD.print(msg2);
}

float getTankVolume(int pin) {
    float sensorValue = getAverage5vRead(pin);
    float current_mA = (sensorValue * 1000.0f / CURRENT_LOOP_RESISTOR);  // Convert sensor reading to mA 
    float pressure = (current_mA - 4.0f) / 16.0f * PRESSURE_SENSOR_MAX;  // Convert mA to psi
    float waterHeight = pressure * PSI_TO_HEIGHT_CONVERSION;  // Convert psi to inches of water column
    waterHeight -= SENSOR_OFFSET_HEIGHT;  // Adjust for sensor location below tank bottom
    float volumeInCubicInches = waterHeight * TANK_CROSS_SECTION_AREA;  // Volume in cubic inches
    float volumeInGallons = volumeInCubicInches / INCHES_CUBIC_TO_GALLONS;  // Convert cubic inches to gallons
    if (volumeInGallons < 0) volumeInGallons = 0;
    return volumeInGallons;
}

float getFlow(int pin){ 
    float sensorValue = getAverage5vRead(pin);
    float current_mA = (sensorValue * 1000.0f / CURRENT_LOOP_RESISTOR);  // Convert sensor reading to mA 
    float flowRate = (current_mA - 4.0f) / 16.0f * FLOW_SENSOR_MAX;  // Convert mA to flow
    if (flowRate < 0) flowRate = 0; 
    //Serial.println(flowRate);
    return flowRate;
}

float getAverage5vRead(int pin){
    int tot { 0 };
    for (auto i = 0u; i < loops; i++)
        tot += Input.analogRead(pin);
    float avg5v = (static_cast<float>(tot) / loops) * toV / rDiv / calibration5v;
    return (avg5v);
}

void updateGallons(){
  unsigned long now = millis();
  unsigned long elapsed = now - timing.lastGallon;
  float gallons = vals.gpm / 60000.0f * elapsed;
  timing.lastGallon = now;
  if (gallons > 0) { 
    vals.gallons = vals.gallons + gallons;
  }
}

void logDataToSD() {
  File dataFile = SD.open(logName, FILE_WRITE);
  if (dataFile) {
      dataFile.print(time(NULL));
      dataFile.print(",");
      dataFile.print(tank1.level);
      dataFile.print(",");
      dataFile.print(tank2.level);
      dataFile.print(",");
      dataFile.print(vals.gpm);
      dataFile.print(",");
      dataFile.print(vals.gallons);
      dataFile.print(",");
      dataFile.println(VBat);
      dataFile.close();
    } else {
      Serial.println("Error opening log file");
  }
}

void checkValveStatus() { //reset status if status and flow don't agree.  Valve handler will retry operation
  if (mainValve.fault == 0) {
    if (vals.gpm > 0.1 && mainValve.status == 0) { //valve not actually closed
      mainValve.status = 1;
      valveRetries++;
    } 
    
    else if (vals.gpm < 0.1 && mainValve.status == 1) { //valve not actually open
      mainValve.status = 0;
      valveRetries++;  
    }
    else {
      mainValve.fault = 1;
      valveRetries = 0;
      lcdUpdateNeeded = true;
    }
  }

  if (valveRetries >= maxRetries) {
    mainValve.fault = 2;
    valveRetries = 0;
    Serial.println("Valve Fault");
  } 
  if (valveRetries > 0) {
    Serial.print("Valve retry #");
    Serial.println(valveRetries);
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

