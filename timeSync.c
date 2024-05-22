void loop() {
  unsigned long board1Time = //TBD
  unsigned long board2Time = //TBD
  unsigned long oneHour = 3600; // One hour in seconds
  unsigned long fiveMinutes = 300; // Five minutes in seconds

  if (board1Time > board2Time || //initial time sync
      (abs(board1Time - board2Time) >= (oneHour - fiveMinutes) &&   //handle daylight savings update 
       abs(board1Time - board2Time) <= (oneHour + fiveMinutes))) {
    rtc.adjust(DateTime(board1Time));
    Serial.println("RTC updated from Board 1");
  }

  delay(60000); // Poll every minute
}
