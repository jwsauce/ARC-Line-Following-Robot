#include <Arduino.h>

#define RPWM_PIN    15
#define LPWM_PIN    16
#define R_EN_PIN    17    // enable forward half-bridge
#define L_EN_PIN    18    // enable reverse half-bridge

// ── LEDC PWM settings ────────────────────────────────
#define PWM_FREQ    10000
#define PWM_RES     8
#define RPWM_CH     0
#define LPWM_CH     1

// ── Motor enable/disable ──────────────────────────────
void motorEnable() {
    digitalWrite(R_EN_PIN, HIGH);
    digitalWrite(L_EN_PIN, HIGH);
    Serial.println("Motor ENABLED");
}

void motorDisable() {
    digitalWrite(R_EN_PIN, LOW);
    digitalWrite(L_EN_PIN, LOW);
    ledcWrite(RPWM_CH, 0);
    ledcWrite(LPWM_CH, 0);
    Serial.println("Motor DISABLED");
}

// ── Motor control functions ───────────────────────────
void motorForward(int speed) {
    ledcWrite(RPWM_CH, speed);
    ledcWrite(LPWM_CH, 0);
}

void motorReverse(int speed) {
    ledcWrite(RPWM_CH, 0);
    ledcWrite(LPWM_CH, speed);
}

void motorBrake() {
    ledcWrite(RPWM_CH, 255);
    ledcWrite(LPWM_CH, 255);
}

void motorCoast() {
    ledcWrite(RPWM_CH, 0);
    ledcWrite(LPWM_CH, 0);
}

// ── Setup ─────────────────────────────────────────────
void setup() {
    Serial.begin(115200);

    // Configure EN pins as output, default LOW (disabled)
    pinMode(R_EN_PIN, OUTPUT);
    pinMode(L_EN_PIN, OUTPUT);
    digitalWrite(R_EN_PIN, LOW);   // motors disabled at startup ✅
    digitalWrite(L_EN_PIN, LOW);

    // Configure LEDC
    ledcSetup(RPWM_CH, PWM_FREQ, PWM_RES);
    ledcSetup(LPWM_CH, PWM_FREQ, PWM_RES);
    ledcAttachPin(RPWM_PIN, RPWM_CH);
    ledcAttachPin(LPWM_PIN, LPWM_CH);

    // Ensure PWM is zero before enabling
    motorCoast();

    Serial.println("System ready. Motors disabled.");
    Serial.println("Waiting 2 seconds before enabling...");
    delay(2000);

    // Explicitly enable motors after safe initialisation
    motorEnable();
    delay(500);

    Serial.println("Starting test sequence...");
}

// ── Test sequence ─────────────────────────────────────
void loop() {

    // Test 1: Ramp forward
    Serial.println("Test 1: Ramp FORWARD");
    for (int spd = 0; spd <= 255; spd += 5) {
        motorForward(spd);
        delay(30);
    }
    delay(1000);

    // Test 2: Full forward
    Serial.println("Test 2: Full FORWARD 2s");
    motorForward(255);
    delay(2000);

    // Test 3: Brake
    Serial.println("Test 3: BRAKE");
    motorBrake();
    delay(1000);

    // Test 4: Ramp reverse
    Serial.println("Test 4: Ramp REVERSE");
    for (int spd = 0; spd <= 255; spd += 5) {
        motorReverse(spd);
        delay(30);
    }
    delay(1000);

    // Test 5: Full reverse
    Serial.println("Test 5: Full REVERSE 2s");
    motorReverse(255);
    delay(2000);

    // Test 6: Disable motors (simulate E-Stop in firmware)
    Serial.println("Test 6: Software DISABLE motors");
    motorDisable();
    delay(2000);

    // Test 7: Re-enable
    Serial.println("Test 7: Re-ENABLE motors");
    motorEnable();
    delay(500);

    // Test 8: Coast
    Serial.println("Test 8: COAST to stop");
    motorCoast();
    delay(2000);

    Serial.println("Sequence complete. Repeating...\n");
}