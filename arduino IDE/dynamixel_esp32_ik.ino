#include <Arduino.h>

#define DXL_BAUD 1000000

#define DXL_ID2 2   // Shoulder
#define DXL_ID4 4   // Elbow

#define WRITE_DATA     0x03
#define GOAL_POSITION  0x1E
#define TORQUE_ENABLE  0x18

#define TX2_PIN 17
#define RX2_PIN 16

HardwareSerial DXL(2);

// Link lengths (mm)
float L1 = 150.0f;
float L2 = 150.0f;

void enableTorque(uint8_t id);
void sendDynamixelPosition(uint8_t id, int position);
void computeIK(float x, float y);

void setup() {

  Serial.begin(115200);
  DXL.begin(DXL_BAUD, SERIAL_8N1, RX2_PIN, TX2_PIN);

  delay(1000);

  enableTorque(DXL_ID2);
  enableTorque(DXL_ID4);

  Serial.println("Enter X Y:");
}

void loop() {

  if (Serial.available()) {

    float x = Serial.parseFloat();
    float y = Serial.parseFloat();

    computeIK(x, y);

    Serial.println("Enter next X Y:");
  }
}

void computeIK(float x, float y) {

  float r2 = x*x + y*y;
  float r  = sqrt(r2);

  if (r > (L1 + L2) || r < fabs(L1 - L2)) {
    Serial.println("Unreachable");
    return;
  }

  float cos_theta2 = (r2 - L1*L1 - L2*L2) / (2.0f * L1 * L2);
  cos_theta2 = constrain(cos_theta2, -1.0f, 1.0f);

  float theta2 = acos(cos_theta2);

  float a = atan2(y, x);
  float theta0 = asin((L2 * sin(theta2)) / r);
  float theta1 = a - theta0;

  float theta1_deg = theta1 * 180.0f / PI;
  float theta2_deg = theta2 * 180.0f / PI;

  // AX-12 center offset
  theta1_deg += 150.0f;
  theta2_deg += 150.0f;

  int dxl2 = theta1_deg * (1023.0f / 300.0f);
  int dxl4 = theta2_deg * (1023.0f / 300.0f);

  dxl2 = constrain(dxl2, 50, 900); //max allowed and min allowed value
  dxl4 = constrain(dxl4, 50, 900);

  Serial.print("ID2: ");
  Serial.println(dxl2);
  Serial.print("ID4: ");
  Serial.println(dxl4);

  sendDynamixelPosition(DXL_ID2, dxl2);
  sendDynamixelPosition(DXL_ID4, dxl4);
}

void enableTorque(uint8_t id) {

  uint8_t length = 4;
  uint8_t value = 1;

  uint8_t checksum =
    ~(id + length + WRITE_DATA + TORQUE_ENABLE + value);

  DXL.write(0xFF);
  DXL.write(0xFF);
  DXL.write(id);
  DXL.write(length);
  DXL.write(WRITE_DATA);
  DXL.write(TORQUE_ENABLE);
  DXL.write(value);
  DXL.write(checksum);
}

void sendDynamixelPosition(uint8_t id, int position) {

  uint8_t pos_L = position & 0xFF;
  uint8_t pos_H = (position >> 8) & 0xFF;

  uint8_t length = 5;

  uint8_t checksum =
    ~(id + length + WRITE_DATA + GOAL_POSITION + pos_L + pos_H);

  DXL.write(0xFF);
  DXL.write(0xFF);
  DXL.write(id);
  DXL.write(length);
  DXL.write(WRITE_DATA);
  DXL.write(GOAL_POSITION);
  DXL.write(pos_L);
  DXL.write(pos_H);
  DXL.write(checksum);
}
