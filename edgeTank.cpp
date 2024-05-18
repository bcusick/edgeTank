/*
  Arduino Edge Control - RTC Alarm Example

  This sketch shows how to use the RTC (PCF8563T) on the Arduino
  Edge Control and how to configure and use the RTC's alarm.

  Circuit:
   - Arduino Edge Control
   - CR2032 Battery
   - Optional: Arduino Edge Control LCD + Button brekout

*/

#include "Helpers.h"
#include <Arduino_EdgeControl.h>
#include "shared.hpp"

// The MKR1 board I2C address
#define MKR1_I2C_ADDR 0x05

SensorValues_t sensorData; //stucture to hold All data passed between Edge and NB

void pwrUp ();
void pwrDown ();

constexpr uint32_t printInterval { 1000 };
uint32_t printNow { 0 };
int status12v = 0;
float UVLO = 11.0;
float UVLO_restart = 11.5;
auto then = time(NULL);

void setup()
{
  EdgeControl.begin();
  Wire.begin();
  Serial.begin(115200);
  delay(500);
 
  delay(500);
  Power.on(PWR_3V3);
  while (!Power.status(PWR_3V3));
  Power.on(PWR_VBAT);
  while(!Power.status(PWR_VBAT));
  Power.on(PWR_MKR1);
  Power.on(PWR_19V);
   
  // Init Edge Control IO Expander
  Serial.print("IO Expander initializazion ");
  if (!Expander.begin()) {
    Serial.println("failed.");
    Serial.println("Please, be sure to enable gated 3V3 and 5V power rails");
    Serial.println("via Power.enable3V3() and Power.enable5V().");
  } else Serial.println("succeeded.");

  // LCD button definition
  //pinMode(POWER_ON, INPUT);
  //attachInterrupt(POWER_ON, buttonPress, RISING);

  // Arduino Edge Control ports init
  //Input.begin();
  //Input.enable();
  //Latching.begin();

  //analogReadResolution(adcResolution);

  // set up the LCD's number of columns and rows:
  LCD.begin(16, 2);
  LCD.backlight();
  //setSystemClock(__DATE__, __TIME__);  // define system time as a reference for the RTC

  //printNow = time(NULL); //change to RTCauto rtcTime = time(NULL
  
  //initBoard();
  
}

void loop(){
    auto vbat = Power.getVBat();
    
    //Serial.println("looping...");
    

    LCD.setCursor(0, 0);
    LCD.print("Battery: ");
    
    LCD.setCursor(9, 0);
    LCD.print(vbat);
    LCD.setCursor(14, 0);
    LCD.print("V");
    

    getCloudValues();
    Serial.println(sensorData.epoch);
    set_time(sensorData.epoch);

    LCD.setCursor(0, 1);
    LCD.print(getLocalhour());
        
        
    if (vbat < UVLO){
      Serial.println("battery offline");
      pwrDown();
      while(vbat < UVLO_restart){
        Power.on(PWR_VBAT);
        delay(5000);
        vbat = Power.getVBat();
        Serial.println(vbat);
        Power.off(PWR_VBAT);   
        delay(5000); //check once a minute
      }
      pwrUp();
      Serial.println("battery online");
      LCD.begin(16, 2);
      LCD.backlight();
      
    }

      //while(!Power.status(PWR_VBAT)){
        
}
      
    

void pwrUp () {
  
  Power.on(PWR_3V3);
  Power.on(PWR_VBAT);
  Power.on(PWR_MKR1);
  Power.on(PWR_19V);
  delay(1000);

}

void pwrDown () { //need to decide what to power down on low battery.  Still want to datalog and 4-20ma data?
  
  Power.off(PWR_3V3);
  Power.off(PWR_VBAT);
  Power.off(PWR_MKR1);
  Power.off(PWR_19V);
  delay(1000);

}

/*
// Function to send SensorValues_t structure
void sendSensorValues(const SensorValues_t &values) {
  Wire.beginTransmission(MKR1_I2C_ADDR); // Slave address
  Wire.write((byte *)&values, sizeof(values));
  Wire.endTransmission();
}


// Function to receive SensorValues_t structure
void receiveSensorValues(SensorValues_t &values) {
  Wire.requestFrom(8, sizeof(values)); // Master's address and size of data
  byte *ptr = (byte *)&values;
  for (int i = 0; i < sizeof(values); i++) {
    *ptr++ = Wire.read();
  }
}

// Callback function for request event
void requestEvent() {
  
  // Get sensor data (e.g., from sensors connected to this Arduino)
  // Populate sensorData with actual sensor readings
  // For now, let's just send some dummy values
  sensorData.temperature = 25.5;
  sensorData.humidity = 50.5;
  sensorData.pressure = 1013;

  // Send sensor data to master
  Wire.write((byte *)&sensorData, sizeof(sensorData));
}
*/
void getCloudValues() {

  Wire.beginTransmission(MKR1_I2C_ADDR);  //slave's 7-bit I2C address

  // asking if the MKR board is connected to be able to communicate
  byte busStatus = Wire.endTransmission();

  if (busStatus != 0) {  
    Serial.println("Can't find I2C");// if the slave is not connected this happens

  } else {  // if the slave is connected this happens

    Wire.requestFrom(MKR1_I2C_ADDR, sizeof(SensorValues_t));

    uint8_t buf[200];
    for (uint8_t i = 0; i < sizeof(SensorValues_t); i++)
      if (Wire.available())
        buf[i] = Wire.read();

    SensorValues_t *values = (SensorValues_t *)buf;

    sensorData.epoch = values->epoch; //time
    }

  }