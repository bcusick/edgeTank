#pragma once

// Define the tasks
void task1() {
  Serial.println("Executing Task 1");
}

void task2() {
  Serial.println("Executing Task 2");
}

void task3() {
  Serial.println("Executing Task 3");
}

// Define the schedule (hours) and the corresponding tasks
const int scheduleSize = 3;
int taskHours[scheduleSize] = {
	8, 
	14, 
	20,
	}; // Hours when tasks should be executed

void (*tasks[scheduleSize])() = {
	task1, 
	task2, 
	task3
	}; // Array of function pointers to tasks

void setup() {
  Serial.begin(9600);
  setTime(7, 59, 50, 1, 1, 2022); // Set initial time for testing
}

void loop() {
  int currentHour = hour();
  
  // Check the schedule
  for (int i = 0; i < scheduleSize; i++) {
    if (currentHour == taskHours[i]) {
      tasks[i](); // Execute the scheduled task
      delay(3600000); // Wait for an hour to prevent repeated execution in the same hour
    }
  }

  delay(1000); // Check the schedule every second
}

