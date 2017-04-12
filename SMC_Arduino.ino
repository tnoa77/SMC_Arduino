#include <Stepper.h>
#include <SoftwareSerial.h>

// PIN DEFINE START
#define LIGHT_GATE      2
#define YD_WRITE_ENABLE 3
#define YD_VCC          4

#define TRIGGER         5

#define SWITCHA         6
#define SWITCHB         7

#define L298N_IN1       8
#define L298N_IN2       9
#define L298N_IN3       10
#define L298N_IN4       11

#define SERIAL_RX       12
#define SERIAL_TX       13
// PIN DEFINE END

#define BYTE            unsigned char
#define YD_STATUS_RUN   0
#define YD_STATUS_STOP  1
#define YD_STATUS_HOLD  2

#define MOTOR_STEP      200
#define MOTOR_HEIGHT    50

#define PromaType int32_t
#define PromaParam int32_t

#define PT_MOTOR  0x00
#define PT_THERM  0x01
#define PT_DELAY  0x02
#define PT_SWITCHA  0x03
#define PT_SWITCHB  0x04
#define PT_TRIGGER  0x05

#define PT_MOTOR_UP   0x00
#define PT_MOTOR_DOWN 0x01
#define PT_SWITCH_CUT  0x00
#define PT_SWITCH_CLOSURE 0x01

struct PromaItem {
  PromaType promaType;
  PromaParam wParam;
  PromaParam lParam;
};

void motorInit();
void motorDown(bool down);
void setYdTemperature(int t);
void setYdRate(int t);
void setYdStatus(int status);

Stepper stepper(MOTOR_STEP, L298N_IN1, L298N_IN2, L298N_IN3, L298N_IN4);
SoftwareSerial ydSerial(SERIAL_RX, SERIAL_TX);
PromaItem item;
bool runningFlag = false;

void setup() {
  pinMode(LIGHT_GATE, INPUT);
  pinMode(YD_VCC, OUTPUT);
  pinMode(TRIGGER, INPUT);
  pinMode(YD_WRITE_ENABLE, OUTPUT);
  pinMode(SWITCHA, OUTPUT);
  pinMode(SWITCHB, OUTPUT);
  digitalWrite(YD_VCC, HIGH);
  digitalWrite(YD_WRITE_ENABLE, HIGH);
  digitalWrite(SWITCHA, LOW);
  digitalWrite(SWITCHB, LOW);
  Serial.begin(9600);
  Serial.println(sizeof(PromaItem));
  ydSerial.begin(9600);
  stepper.setSpeed(450);
  motorInit();
  delay(2500);
}

void loop() {
  if(digitalRead(TRIGGER) == LOW && !runningFlag) {
    delay(10);
    if(digitalRead(TRIGGER) == LOW && !runningFlag) {
      runningFlag = true;
      Serial.write(0x42);
      Serial.write(0x4d);
    }
  }
  if(digitalRead(TRIGGER) == HIGH && runningFlag) {
    delay(10);
    if(digitalRead(TRIGGER) == HIGH && runningFlag) {
      runningFlag = false;
    }
  }
  if(Serial.available() > 0) {
    Serial.readBytes((char*) &item, sizeof(PromaItem));
    Serial.write((char*) &item, sizeof(PromaItem));
    switch (item.promaType)
    {
    case PT_MOTOR:
      motorDown(item.wParam);
      break;
    case PT_THERM:
      setYdTemperature(item.wParam);
      setYdRate(item.wParam);
      setYdStatus(YD_STATUS_RUN);
      break;
    case PT_SWITCHA:
      digitalWrite(SWITCHA, item.wParam == PT_SWITCH_CLOSURE ? HIGH : LOW);
      break;
    case PT_SWITCHB:
      digitalWrite(SWITCHB, item.wParam == PT_SWITCH_CLOSURE ? HIGH : LOW);
      break;
    default:
      break;
    }
  }
}

void motorInit() {
  if(digitalRead(LIGHT_GATE)) {
    for(int i = 0; i < 10; ++i)
      stepper.step(-MOTOR_STEP);
    delay(500);
  }
  while(true) {
    if(digitalRead(LIGHT_GATE))
      break;
    stepper.step(MOTOR_STEP);
  }
  for(int i = 0; i < 4; ++i)
      stepper.step(MOTOR_STEP);
}

void motorDown(bool down) {
  if (down)
    for(int i = 0; i < MOTOR_HEIGHT; ++i)
      stepper.step(-MOTOR_STEP);
  else
    motorInit();
}

void setYdTemperature(int t) {
  t *= 10;
  int checksum = 0 * 256 + 67 + t + 1;
  BYTE val[8] = { 0x81, 0x81, 0x43, 0x00, (BYTE) (t % 0x100), (BYTE) (t / 0x100), (BYTE) (checksum % 0x100), (BYTE) (checksum / 0x100) };
  ydSerial.write(val, 8);
}

void setYdRate(int t) {
  t *= 10;
  int checksum = 0x2A * 256 + 67 + t + 1;
  BYTE val[8] = { 0x81, 0x81, 0x43, 0x2A, (BYTE) (t % 0x100), (BYTE) (t / 0x100), (BYTE) (checksum % 0x100), (BYTE) (checksum / 0x100) };
  ydSerial.write(val, 8);
}

void setYdStatus(int status) {
  BYTE val[3][8] = {
    { 0x81, 0x81, 0x43, 0x1B, 0x00, 0x00, 0x44, 0x1B },   // run
    { 0x81, 0x81, 0x43, 0x1B, 0x01, 0x00, 0x45, 0x1B },   // stop
    { 0x81, 0x81, 0x43, 0x1B, 0x02, 0x00, 0x46, 0x1B }    // hold
  };
  ydSerial.write(val[status], 8);
}



