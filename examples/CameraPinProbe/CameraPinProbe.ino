#include <Arduino.h>
#include <Wire.h>

namespace {

constexpr int XCLK_PIN = 33;
constexpr int XCLK_CHANNEL = 0;
constexpr uint32_t XCLK_FREQ_HZ = 20000000;

constexpr int CAMERA_PWDN_PIN = -1;
constexpr int CAMERA_RESET_PIN = -1;

constexpr uint8_t CAMERA_SCCB_ADDR = 0x30;

struct PinPair {
  const char *name;
  int sda;
  int scl;
};

const PinPair kCandidates[] = {
    {"ESP32-S3-CAM-TS", 37, 36},
    {"GENERIC_S3_CAM", 17, 18},
    {"ESP32-S3-EYE", 4, 5},
};

void startXclk() {
  const bool attached = ledcAttach(XCLK_PIN, XCLK_FREQ_HZ, 1);
  if (!attached) {
    Serial.println("Failed to attach LEDC for XCLK");
    return;
  }
  ledcWrite(XCLK_PIN, 1);
}

void powerUpCamera() {
  if (CAMERA_PWDN_PIN >= 0) {
    pinMode(CAMERA_PWDN_PIN, OUTPUT);
    digitalWrite(CAMERA_PWDN_PIN, LOW);
  }

  if (CAMERA_RESET_PIN >= 0) {
    pinMode(CAMERA_RESET_PIN, OUTPUT);
    digitalWrite(CAMERA_RESET_PIN, LOW);
    delay(10);
    digitalWrite(CAMERA_RESET_PIN, HIGH);
  }
}

bool probeAddress(TwoWire &wire, uint8_t address) {
  wire.beginTransmission(address);
  return wire.endTransmission() == 0;
}

void scanBus(TwoWire &wire) {
  bool foundAny = false;
  for (uint8_t address = 1; address < 127; ++address) {
    wire.beginTransmission(address);
    if (wire.endTransmission() == 0) {
      Serial.printf("  Found device at 0x%02X\n", address);
      foundAny = true;
    }
  }

  if (!foundAny) {
    Serial.println("  No I2C devices found");
  }
}

void probePair(const PinPair &pair) {
  Wire.end();
  delay(20);

  Serial.printf("\nTrying %s: SDA=%d SCL=%d\n", pair.name, pair.sda, pair.scl);
  Wire.begin(pair.sda, pair.scl, 100000);
  delay(50);

  const bool cameraAck = probeAddress(Wire, CAMERA_SCCB_ADDR);
  Serial.printf("  Probe 0x%02X: %s\n", CAMERA_SCCB_ADDR,
                cameraAck ? "ACK" : "no response");
  scanBus(Wire);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("Camera SCCB pin probe starting...");
  Serial.printf("Starting XCLK on GPIO%d at %u Hz\n", XCLK_PIN, XCLK_FREQ_HZ);
  startXclk();
  powerUpCamera();
  delay(200);

  for (const auto &pair : kCandidates) {
    probePair(pair);
  }

  Serial.println("\nProbe complete.");
  Serial.println("If all pairs fail, verify camera power, XCLK, and any hidden PWDN/RESET pins.");
}

void loop() {
  delay(1000);
}
