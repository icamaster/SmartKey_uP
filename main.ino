#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"

// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
#include "Wire.h"
#endif
#include <SoftwareSerial.h>
SoftwareSerial bluetooth(10, 11); // RX, TX

// class default I2C address is 0x68
// specific I2C addresses may be passed as a parameter here
// AD0 low = 0x68 (default for SparkFun breakout and InvenSense evaluation board)
// AD0 high = 0x69
MPU6050 mpu;
//MPU6050 mpu(0x69); // <-- use for AD0 high

// MPU control/status vars
bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer

// orientation/motion vars
Quaternion q;           // [w, x, y, z]         quaternion container
VectorInt16 aa;         // [x, y, z]            accel sensor measurements
VectorInt16 aaReal;     // [x, y, z]            gravity-free accel sensor measurements
VectorInt16 aaWorld;    // [x, y, z]            world-frame accel sensor measurements
VectorFloat gravity;    // [x, y, z]            gravity vector
float euler[3];         // [psi, theta, phi]    Euler angle container
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector

//1 - Steady
//2 - Rotate left
//3 - Rotate Right
//4 - Shake
//5 - Rotate Up 
//6 - Rotate Down
int lastStatus = 1;
String inputPassword = "";

//Switch on pin 12
const int buttonPin = 12;
int buttonReading;
long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 50;    // the debounce time; increase if the output flickers
bool firstPush = true;
bool sendEnd = false;
int lastButtonState = HIGH;
float ypr_1_start;
float ypr_2_start;
bool lastWasShake = false; // boolean to avoid multiple shakes read error



// ================================================================
// ===               INTERRUPT DETECTION ROUTINE                ===
// ================================================================

volatile bool mpuInterrupt = false;     // indicates whether MPU interrupt pin has gone high
void dmpDataReady() {
  mpuInterrupt = true;
}
// ================================================================
// ===                      INITIAL SETUP                       ===
// ================================================================

void setup() {
  //Set switch pin as input
  pinMode(buttonPin, INPUT);
  //Set internal pull-up resistor
  digitalWrite(buttonPin, HIGH);
  // join I2C bus (I2Cdev library doesn't do this automatically)
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
  Wire.begin();
  TWBR = 24; // 400kHz I2C clock (200kHz if CPU is 8MHz)
#elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
  Fastwire::setup(400, true);
#endif

  // initialize serial communication
  Serial.begin(115200);
  while (!Serial); // wait for Leonardo enumeration, others continue immediately
  bluetooth.begin(9600);

  // initialize device
  Serial.println(F("Initializing I2C devices..."));
  mpu.initialize();

  // verify connection
  Serial.println(F("Testing device connections..."));
  Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));

  // load and configure the DMP
  Serial.println(F("Initializing DMP..."));
  devStatus = mpu.dmpInitialize();

  // supply your own gyro offsets here, scaled for min sensitivity
  mpu.setXGyroOffset(36);//220
  mpu.setYGyroOffset(-87);//79
  mpu.setZGyroOffset(-29);//-85
  mpu.setZAccelOffset(1788); // 1688 factory default for my test chip

  // make sure it worked (returns 0 if so)
  if (devStatus == 0) {
    // turn on the DMP, now that it's ready
    Serial.println(F("Enabling DMP..."));
    mpu.setDMPEnabled(true);

    // enable Arduino interrupt detection
    Serial.println(F("Enabling interrupt detection (Arduino external interrupt 0)..."));
    attachInterrupt(0, dmpDataReady, RISING);
    mpuIntStatus = mpu.getIntStatus();

    // set our DMP Ready flag so the main loop() function knows it's okay to use it
    Serial.println(F("DMP ready! Waiting for first interrupt..."));
    dmpReady = true;

    // get expected DMP packet size for later comparison
    packetSize = mpu.dmpGetFIFOPacketSize();
  } else {
    // ERROR!
    // 1 = initial memory load failed
    // 2 = DMP configuration updates failed
    // (if it's going to break, usually the code will be 1)
    Serial.print(F("DMP Initialization failed (code "));
    Serial.print(devStatus);
    Serial.println(F(")"));
  }

}



// ================================================================
// ===                    MAIN PROGRAM LOOP                     ===
// ================================================================

void loop() {

  // if programming failed, don't try to do anything
  if (!dmpReady) return;

  // wait for MPU interrupt or extra packet(s) available
  while (!mpuInterrupt && fifoCount < packetSize) {
  }

  // reset interrupt flag and get INT_STATUS byte
  mpuInterrupt = false;
  mpuIntStatus = mpu.getIntStatus();

  // get current FIFO count
  fifoCount = mpu.getFIFOCount();

  // check for overflow (this should never happen unless our code is too inefficient)
  if ((mpuIntStatus & 0x10) || fifoCount == 1024) {
    // reset so we can continue cleanly
    mpu.resetFIFO();
    Serial.println(F("FIFO overflow!"));

    // otherwise, check for DMP data ready interrupt (this should happen frequently)
  } else if (mpuIntStatus & 0x02) {
    // wait for correct available data length, should be a VERY short wait
    while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();

    // read a packet from FIFO
    mpu.getFIFOBytes(fifoBuffer, packetSize);

    // track FIFO count here in case there is > 1 packet available
    // (this lets us immediately read more without waiting for an interrupt)
    fifoCount -= packetSize;

    // display Euler angles in degrees
    mpu.dmpGetQuaternion(&q, fifoBuffer);
    mpu.dmpGetAccel(&aa, fifoBuffer);
    mpu.dmpGetGravity(&gravity, &q);
    mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
    mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
    mpu.dmpGetLinearAccelInWorld(&aaWorld, &aaReal, &q);
    /*
        Serial.print("ypr\t");
        Serial.print(ypr[0] * 180 / M_PI);
        Serial.print("\t");
        Serial.print(ypr[1] * 180 / M_PI);
        Serial.print("\t");
        Serial.println(ypr[2] * 180 / M_PI);

        Serial.print(aaWorld.x);
        Serial.print("\t");
        Serial.print(aaWorld.y);
        Serial.print("\t");
        Serial.println(aaWorld.z);
    */
    checkButton();

    if (buttonReading == LOW) {
      if (firstPush == true) {
        ypr_1_start = ypr[1];
        ypr_2_start = ypr[2];
        inputPassword = "";
        firstPush = false;
        Serial.println("Start");
        bluetooth.println("Start");
      }
      else {
        int currentKeyStatus = checkKeyStatus(ypr[1], ypr_1_start, ypr[2], ypr_2_start, aaWorld.x);
        if (currentKeyStatus != lastStatus) {
          if (currentKeyStatus != 1) //Send only keyStatuses diferent than 1
            inputPassword += currentKeyStatus;
          lastStatus = currentKeyStatus;
        }
      }
    }
    if (sendEnd == true) {
      bluetooth.println("Password:" + inputPassword);
      Serial.println("Password:" + inputPassword);
      sendEnd = false;
    }
  }
}
int checkKeyStatus(float ypr_1, float ypr_1_start, float ypr_2, float ypr_2_start, float acc_X) {
  //Initial key status; 1=Standby
  int keyStatus = 1;
  //Calculate the angle between starting point and current point for X
  float ypr_1_diff = ypr_1 - ypr_1_start;
  ypr_1_diff *= 180 / M_PI;
  //Serial.println(ypr_1_diff);
  //Calculate the angle between starting point and current point for Z
  float ypr_2_diff = ypr_2 - ypr_1_start;
  ypr_2_diff *= 180 / M_PI;
  //Serial.println(ypr_2_diff);

  if (acc_X > 10000 && lastWasShake == false) {
    keyStatus = 4;
    lastWasShake = true;
    //Serial.println("Shake");
  }
  else if (ypr_1_diff < -50) {
    keyStatus = 2;
    lastWasShake = false; 
    //Serial.println("R Left");
  }
  else if (ypr_1_diff > 50) {
    //Serial.println("R Right");
    keyStatus = 3;
    lastWasShake = false; 
  }
  else if (ypr_2_diff < -50) {
    keyStatus = 5;
    lastWasShake = false; 
    //Serial.println("R Up");
  }
  else if (ypr_2_diff > 50) {
    //Serial.println("R Down");
    keyStatus = 6;
    lastWasShake = false; 
  }
  else {
    keyStatus = 1;
    //Serial.println("Steady");
  }
  return keyStatus;
}

void checkButton() {
  if (millis() - lastDebounceTime > debounceDelay) {
    buttonReading = digitalRead(buttonPin);
    lastDebounceTime = millis;
    if (lastButtonState != buttonReading) {
      if (lastButtonState == HIGH) {
        //Serial.println("Last State = HIGH");
        firstPush = true;
        sendEnd = false;
      }
      else if (lastButtonState == LOW) {
        //Serial.println("Last State = LOW");
        firstPush = false;
        sendEnd = true;
      }
      lastButtonState = buttonReading;
    }
  }
}

