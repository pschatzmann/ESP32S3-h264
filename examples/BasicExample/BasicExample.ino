/* Basic example for ESP_H264_Arduino
 * Prints version string from the library.
 */

#include "h264/esp_h264_arduino.h"
#include "h264/esp_h264_version.h"

void setup() {
  Serial.begin(115200);
  while (!Serial) ;
  const char *v = esp_h264_get_version();
  Serial.printf("ESP_H264 version: %s\n", v ? v : "<unknown>");
}

void loop() {
  delay(1000);
}
