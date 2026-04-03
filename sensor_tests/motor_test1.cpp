#include <Arduino.h>

// ── Left IBT-2 Motor Driver ────────────────────────────
#define L_RPWM_PIN   17
#define L_LPWM_PIN   18
#define L_R_EN_PIN   8
#define L_L_EN_PIN   3

// ── Right IBT-2 Motor Driver ───────────────────────────
#define R_RPWM_PIN   2
#define R_LPWM_PIN   1
#define R_R_EN_PIN   42
#define R_L_EN_PIN   41

// ── LEDC PWM Settings ─────────────────────────────────
#define PWM_FREQ     10000
#define PWM_RES      8          // 8-bit → 0–255
#define L_RPWM_CH    0
#define L_LPWM_CH    1
#define R_RPWM_CH    2
#define R_LPWM_CH    3

// ── Tuning ────────────────────────────────────────────
#define DRIVE_SPEED   200       // 0–255 cruise speed
#define SPIN_SPEED    180       // 0–255 spin speed

// Tune this until one full rotation is achieved
#define SPIN_360_MS   2000      // ms for one 360° spin at SPIN_SPEED

// ═════════════════════════════════════════════════════
//  LOW-LEVEL DRIVERS
// ═════════════════════════════════════════════════════

void motorsEnable() {
    digitalWrite(L_R_EN_PIN, HIGH);
    digitalWrite(L_L_EN_PIN, HIGH);
    digitalWrite(R_R_EN_PIN, HIGH);
    digitalWrite(R_L_EN_PIN, HIGH);
    Serial.println("[EN] Motors ENABLED");
}

void motorsDisable() {
    // Kill PWM first, then pull EN low — safest order
    ledcWrite(L_RPWM_CH, 0);  ledcWrite(L_LPWM_CH, 0);
    ledcWrite(R_RPWM_CH, 0);  ledcWrite(R_LPWM_CH, 0);
    digitalWrite(L_R_EN_PIN, LOW);
    digitalWrite(L_L_EN_PIN, LOW);
    digitalWrite(R_R_EN_PIN, LOW);
    digitalWrite(R_L_EN_PIN, LOW);
    Serial.println("[EN] Motors DISABLED");
}

// Raw write to one driver: fwd and rev PWM values
inline void setLeft(uint8_t fwd, uint8_t rev) {
    ledcWrite(L_RPWM_CH, fwd);
    ledcWrite(L_LPWM_CH, rev);
}

inline void setRight(uint8_t fwd, uint8_t rev) {
    ledcWrite(R_RPWM_CH, fwd);
    ledcWrite(R_LPWM_CH, rev);
}

// ═════════════════════════════════════════════════════
//  HIGH-LEVEL MOTION PRIMITIVES
// ═════════════════════════════════════════════════════

void driveForward(int speed) {
    setLeft(speed, 0);
    setRight(speed, 0);
}

void driveReverse(int speed) {
    setLeft(0, speed);
    setRight(0, speed);
}

// Coast — zero torque, motor spins freely
void coast() {
    setLeft(0, 0);
    setRight(0, 0);
}

// Hard brake — both sides driven high → short-circuit braking
void brake() {
    setLeft(255, 255);
    setRight(255, 255);
}

// Spin in place: left fwd + right rev = clockwise (viewed from above)
void spinCW(int speed) {
    setLeft(speed, 0);
    setRight(0, speed);
}

// Spin in place: left rev + right fwd = anti-clockwise
void spinCCW(int speed) {
    setLeft(0, speed);
    setRight(speed, 0);
}

// Ramp speed linearly from `fromSpeed` down to 0 over `durationMs`
// wasForward: true = was driving forward, false = was driving reverse
void slowStop(int fromSpeed, bool wasForward, unsigned long durationMs) {
    if (fromSpeed <= 0) { coast(); return; }

    unsigned long stepDelay = durationMs / (unsigned long)fromSpeed;

    for (int spd = fromSpeed; spd >= 0; spd--) {
        if (wasForward) driveForward(spd);
        else            driveReverse(spd);
        delay(stepDelay);
    }
    coast();
}

// ═════════════════════════════════════════════════════
//  SETUP
// ═════════════════════════════════════════════════════

void setup() {
    Serial.begin(115200);

    // EN pins — default LOW (safe off)
    pinMode(L_R_EN_PIN, OUTPUT); digitalWrite(L_R_EN_PIN, LOW);
    pinMode(L_L_EN_PIN, OUTPUT); digitalWrite(L_L_EN_PIN, LOW);
    pinMode(R_R_EN_PIN, OUTPUT); digitalWrite(R_R_EN_PIN, LOW);
    pinMode(R_L_EN_PIN, OUTPUT); digitalWrite(R_L_EN_PIN, LOW);

    // LEDC channel setup
    ledcSetup(L_RPWM_CH, PWM_FREQ, PWM_RES);
    ledcSetup(L_LPWM_CH, PWM_FREQ, PWM_RES);
    ledcSetup(R_RPWM_CH, PWM_FREQ, PWM_RES);
    ledcSetup(R_LPWM_CH, PWM_FREQ, PWM_RES);

    // Attach pins to channels
    ledcAttachPin(L_RPWM_PIN, L_RPWM_CH);
    ledcAttachPin(L_LPWM_PIN, L_LPWM_CH);
    ledcAttachPin(R_RPWM_PIN, R_RPWM_CH);
    ledcAttachPin(R_LPWM_PIN, R_LPWM_CH);

    // Zero PWM before enabling
    coast();

    Serial.println("System ready. Motors disabled.");
    Serial.println("Enabling in 2 seconds...");
    delay(2000);

    motorsEnable();
    delay(500);
    Serial.println("Starting sequence...\n");
}

// ═════════════════════════════════════════════════════
//  MAIN SEQUENCE
// ═════════════════════════════════════════════════════

void loop() {

    // ── 1. Drive forward 2s ──────────────────────────
    Serial.println("1. Drive FORWARD 2s");
    driveForward(DRIVE_SPEED);
    delay(2000);

    // ── 2. Slowly stop in 2s ─────────────────────────
    Serial.println("2. Slow STOP (forward → 0) over 2s");
    slowStop(DRIVE_SPEED, true, 2000);

    // ── 3. Stop entirely for 2s ──────────────────────
    Serial.println("3. COAST — stopped 2s");
    coast();
    delay(2000);

    // ── 4. Drive backward 2s ─────────────────────────
    Serial.println("4. Drive REVERSE 2s");
    driveReverse(DRIVE_SPEED);
    delay(2000);

    // ── 5. Slowly stop in 2s ─────────────────────────
    Serial.println("5. Slow STOP (reverse → 0) over 2s");
    slowStop(DRIVE_SPEED, false, 2000);

    // ── 6. Stop entirely for 2s ──────────────────────
    Serial.println("6. COAST — stopped 2s");
    coast();
    delay(2000);

    // ── 7. Drive forward 2s ──────────────────────────
    Serial.println("7. Drive FORWARD 2s");
    driveForward(DRIVE_SPEED);
    delay(2000);

    // ── 8. Brake suddenly ────────────────────────────
    Serial.println("8. HARD BRAKE");
    brake();
    delay(800);     // hold brake briefly, then coast
    coast();
    delay(1200);

    // ── 9. Drive backward 2s ─────────────────────────
    Serial.println("9. Drive REVERSE 2s");
    driveReverse(DRIVE_SPEED);
    delay(2000);

    // ── 10. Brake suddenly ───────────────────────────
    Serial.println("10. HARD BRAKE");
    brake();
    delay(800);
    coast();
    delay(1200);

    // ── 11. Spin 360° clockwise ──────────────────────
    Serial.println("11. Spin 360° CLOCKWISE");
    spinCW(SPIN_SPEED);
    delay(SPIN_360_MS);
    coast();
    delay(1000);

    // ── 12. Spin 360° anti-clockwise ─────────────────
    Serial.println("12. Spin 360° ANTI-CLOCKWISE");
    spinCCW(SPIN_SPEED);
    delay(SPIN_360_MS);
    coast();
    delay(2000);

    Serial.println("Sequence complete. Repeating...\n");
}