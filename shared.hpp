#pragma once

typedef struct {

	bool connected = true;  //mobile connection
	unsigned long epoch = 0; //current time taken from modem

  float gpm = 0;
  float gallons = 0;
  bool isReset = true;
} SensorValues_t;

typedef struct {
  float lastGpmIN  = 0;
  float lastGallon = 0;
} TimeKeeping_t;

typedef struct {
  float sensorTotal  = 0;
  float level = 0;
  int readings[60];  // Array to store the readings
} TankValues_t;

typedef struct {
	bool status 	= false; //false is closed
	bool cmd	 	= false;
	int fault = 0; 
  bool autoControl = true;
} Valves_t;