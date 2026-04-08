#include <Arduino.h>
#include <ESP32Servo.h>

// ── CONFIGURATION ──────────────────────────────────────────

// Color sensor
#define S0_PIN    20
#define S1_PIN    21
#define S2_PIN    10
#define S3_PIN    11 
#define OUT_PIN   9
#define LED_PIN   19

// Left Motor Driver (IBT-2)
#define L_RPWM_PIN    4
#define L_LPWM_PIN    5
#define L_R_EN_PIN    6
#define L_L_EN_PIN    7

// Right Motor Driver (IBT-2)
#define R_RPWM_PIN    37
#define R_LPWM_PIN    38
#define R_R_EN_PIN    36
#define R_L_EN_PIN    35

// Gripper + Arm 
#define GRIPPER_PIN   48 
#define ARM_PIN       47

// IR Sensor Array
const int numSensors = 5;
const int irPins[numSensors] = {1, 2, 42, 41, 40};

// Reset/Start Button
#define BUTTON_PIN    16

// PWM Settings
#define PWM_FREQ      10000
#define PWM_RES       8 
#define L_RPWM_CH     0
#define L_LPWM_CH     1
#define R_RPWM_CH     2
#define R_LPWM_CH     3

// Servo Positions
#define GRIPPER_OPEN   150
#define GRIPPER_CLOSE  75
#define ARM_LIFT       50
#define ARM_DOWN       110

Servo gripper;
Servo arm;

// ── SENSOR HELPERS ─────────────────────────────────────────

void readIRSensors() {
  Serial.print("IR: [ ");
  for (int i = 0; i < numSensors; i++) {
    Serial.print(digitalRead(irPins[i]));
    Serial.print(" ");
  }
  Serial.print("] ");
}

int readColorFreq(int s2, int s3) {
  digitalWrite(S2_PIN, s2);
  digitalWrite(S3_PIN, s3);
  delay(2); 
  return pulseIn(OUT_PIN, LOW, 20000);
}

void readColorSensor() {
  int r = readColorFreq(LOW, LOW);
  int g = readColorFreq(HIGH, HIGH);
  int b = readColorFreq(LOW, HIGH);
  Serial.printf("| Color R:%d G:%d B:%d\n", r, g, b);
}

// ── MOTOR HELPERS ──────────────────────────────────────────

void motorsEnable() {
  digitalWrite(L_R_EN_PIN, HIGH); digitalWrite(L_L_EN_PIN, HIGH);
  digitalWrite(R_R_EN_PIN, HIGH); digitalWrite(R_L_EN_PIN, HIGH);
}

void setMotors(int l_fwd, int l_rev, int r_fwd, int r_rev) {
  ledcWrite(L_RPWM_CH, l_fwd);
  ledcWrite(L_LPWM_CH, l_rev);
  ledcWrite(R_RPWM_CH, r_fwd);
  ledcWrite(R_LPWM_CH, r_rev);
}

void brake() { setMotors(255, 255, 255, 255); delay(200); setMotors(0, 0, 0, 0); }

// This function acts like a delay, but reads sensors while waiting
void monitorAndDelay(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    readIRSensors();
    readColorSensor();
    delay(50); // Sampling rate
  }
}

// ── ARM HELPERS ────────────────────────────────────────────

void pickupSequence() {
  Serial.println("\n>>> ACTION: Picking Up");
  arm.write(ARM_DOWN); delay(800);
  gripper.write(GRIPPER_CLOSE); delay(600);
  arm.write(ARM_LIFT); delay(800);
}

void placeSequence() {
  Serial.println("\n>>> ACTION: Dropping Off");
  arm.write(ARM_DOWN); delay(800);
  gripper.write(GRIPPER_OPEN); delay(600);
  arm.write(ARM_LIFT); delay(800);
}

// ── SETUP ──────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);

  // Initialize IR
  for (int i = 0; i < numSensors; i++) pinMode(irPins[i], INPUT);

  // Initialize Color
  pinMode(S0_PIN, OUTPUT); pinMode(S1_PIN, OUTPUT);
  pinMode(S2_PIN, OUTPUT); pinMode(S3_PIN, OUTPUT);
  pinMode(OUT_PIN, INPUT); pinMode(LED_PIN, OUTPUT);
  digitalWrite(S0_PIN, HIGH); digitalWrite(S1_PIN, LOW); // 20% Scaling
  digitalWrite(LED_PIN, HIGH);

  // Initialize Motors
  pinMode(L_R_EN_PIN, OUTPUT); pinMode(L_L_EN_PIN, OUTPUT);
  pinMode(R_R_EN_PIN, OUTPUT); pinMode(R_L_EN_PIN, OUTPUT);
  ledcSetup(L_RPWM_CH, PWM_FREQ, PWM_RES); ledcAttachPin(L_RPWM_PIN, L_RPWM_CH);
  ledcSetup(L_LPWM_CH, PWM_FREQ, PWM_RES); ledcAttachPin(L_LPWM_PIN, L_LPWM_CH);
  ledcSetup(R_RPWM_CH, PWM_FREQ, PWM_RES); ledcAttachPin(R_RPWM_PIN, R_RPWM_CH);
  ledcSetup(R_LPWM_CH, PWM_FREQ, PWM_RES); ledcAttachPin(R_LPWM_PIN, R_LPWM_CH);
  
  // Initialize Servos
  ESP32PWM::allocateTimer(0); ESP32PWM::allocateTimer(1);
  gripper.attach(GRIPPER_PIN, 500, 2400);
  arm.attach(ARM_PIN, 500, 2400);
  gripper.write(GRIPPER_OPEN); arm.write(ARM_LIFT);

  // Button
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println("--- SYSTEM READY ---");
  Serial.println("Press the button to begin the test sequence.");
}

// ── MAIN LOOP ──────────────────────────────────────────────

void loop() {
  // Wait for button press to start
  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("\n🚀 STARTING TEST SEQUENCE...");
    delay(1000); // Small wait before moving
    motorsEnable();

    // 1. Drive forward for 2s
    Serial.println("STEP 1: Drive Forward 2s");
    setMotors(180, 0, 180, 0); // Speed 180/255
    monitorAndDelay(2000);

    // 2. Brake
    Serial.println("STEP 2: Brake");
    brake();
    monitorAndDelay(500);

    // 3. Arm pick up
    pickupSequence();

    // 4. Drive forward for 2s
    Serial.println("STEP 4: Drive Forward 2s");
    setMotors(180, 0, 180, 0);
    monitorAndDelay(2000);

    // 5. Brake
    Serial.println("STEP 5: Brake");
    brake();
    monitorAndDelay(500);

    // 6. Arm drop object
    placeSequence();

    // 7. Drive forward 2s
    Serial.println("STEP 7: Drive Forward 2s");
    setMotors(180, 0, 180, 0);
    monitorAndDelay(2000);

    // End sequence
    setMotors(0, 0, 0, 0);
    Serial.println("\n✅ TEST SEQUENCE COMPLETE.");
    Serial.println("Press button to repeat.\n");
  }
}