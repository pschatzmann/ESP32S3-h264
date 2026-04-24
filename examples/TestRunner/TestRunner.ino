/* TestRunner example for ESP_H264_Arduino
 * Runs a small set of non-hardware tests adapted from test_case.c
 */

#include "test_case_arduino.h"

void setup() {
  Serial.begin(115200);
  while (!Serial) ;
  Serial.println("Starting ESP_H264 TestRunner");
  run_all_tests();
  Serial.println("All tests finished.");
}

void loop() {
  // nothing
  delay(1000);
}
