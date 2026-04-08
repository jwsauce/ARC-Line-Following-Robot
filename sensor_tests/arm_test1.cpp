// ============================================================
//  Arm + Gripper Controller (ESP32 Version)
//  Force Sensor Removed - Timer Based Gripping
// ============================================================
#include <Arduino.h>
#include <ESP32Servo.h> // ESP32 Servo library

// ── Pins (GPIOs) ─────────────────────────────────────────────
// Note: Changed from 9/10 to 18/19 for ESP32 stability
#define GRIPPER_PIN  48 
#define ARM_PIN      47

// ── Servo Positions ──────────────────────────────────────────
#define GRIPPER_OPEN   150
#define GRIPPER_CLOSE  75
#define ARM_LIFT       50
#define ARM_DOWN       110

// ── Timing ───────────────────────────────────────────────────
#define SERVO_DELAY    800  // Increased slightly for ESP32 torque
#define ACTION_PAUSE   500

Servo gripper;
Servo arm;

// ============================================================
//  HELPERS
// ============================================================
void moveServo(Servo &srv, int pos, const char *label) {
  srv.write(pos);
  Serial.print("[SERVO] ");
  Serial.print(label);
  Serial.print(" -> ");
  Serial.println(pos);
  delay(SERVO_DELAY);
}

// ============================================================
//  PICKUP & PLACE (Simplified - No FSR)
// ============================================================
void pickupSequence() {
  Serial.println("\n===== STARTING PICKUP =====");
  
  // 1. Lower the arm
  moveServo(arm, ARM_DOWN, "Arm DOWN");
  
  // 2. Close the gripper (No longer waiting for force feedback)
  moveServo(gripper, GRIPPER_CLOSE, "Gripper CLOSE");
  delay(ACTION_PAUSE); // Give it a moment to secure the object
  
  // 3. Lift the arm
  moveServo(arm, ARM_LIFT, "Arm LIFT");
  
  Serial.println("===== PICKUP COMPLETE =====\n");
}

void placeSequence() {
  Serial.println("\n===== STARTING PLACE =====");
  
  // 1. Lower the arm
  moveServo(arm, ARM_DOWN, "Arm DOWN");
  
  // 2. Open the gripper
  moveServo(gripper, GRIPPER_OPEN, "Gripper OPEN");
  delay(ACTION_PAUSE);
  
  // 3. Lift the arm back up
  moveServo(arm, ARM_LIFT, "Arm LIFT");
  
  Serial.println("===== PLACE COMPLETE =====\n");
}

// ============================================================
//  SETUP & LOOP
// ============================================================
void setup() {
  // ESP32 usually uses 115200 for the Serial Monitor
  Serial.begin(115200);
  Serial.println("=== ESP32 Arm + Gripper (No FSR) ===");

  // Allow allocation of all timers
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  gripper.setPeriodHertz(50);    // Standard 50hz servo
  arm.setPeriodHertz(50);

  gripper.attach(GRIPPER_PIN, 500, 2400); // Attach with pulse widths
  arm.attach(ARM_PIN, 500, 2400);

  // Initial safe position
  Serial.println("[INIT] Moving to home positions...");
  gripper.write(GRIPPER_OPEN);
  arm.write(ARM_LIFT);
  delay(1000);
  
  Serial.println("[INIT] System Ready.\n");
}

void loop() {
  // ── Step 1: Pick up object ──────────────────────────────
  pickupSequence();

  // ── Step 2: Transit ──────────────────────────────────────
  Serial.println("[MOVE] Simulating movement to drop-off...");
  delay(3000); 

  // ── Step 3: Drop off object ──────────────────────────────
  placeSequence();

  // Wait before starting the cycle again
  Serial.println("[WAIT] Cycle finished. Restarting in 5s...");
  delay(5000);
}