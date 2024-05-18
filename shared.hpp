#pragma once

typedef struct {

	bool connected = true;  //mobile connection
	unsigned long epoch = 0; //current time taken from modem
	float tankLevel_1 = 0;
	float tankLevel_2 = 0;
	bool mainFlow = false;
	bool mainValve = false;
	float temperature = 0;

} SensorValues_t;
 