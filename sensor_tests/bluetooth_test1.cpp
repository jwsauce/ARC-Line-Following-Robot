#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ── Nordic UART Service UUIDs (standard, don't change) ─
#define NUS_SERVICE_UUID  "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_RX_UUID       "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  // phone → ESP32
#define NUS_TX_UUID       "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  // ESP32 → phone

// ── BLE globals ────────────────────────────────────────
BLEServer*          bleServer   = nullptr;
BLECharacteristic*  txChar      = nullptr;
bool                bleConnected = false;

// ── Pin definitions ────────────────────────────────────
#define L_RPWM_PIN   17
#define L_LPWM_PIN   18
#define L_R_EN_PIN   8
#define L_L_EN_PIN   3

#define R_RPWM_PIN   2
#define R_LPWM_PIN   1
#define R_R_EN_PIN   42
#define R_L_EN_PIN   41

// ── LEDC PWM Settings ──────────────────────────────────
#define PWM_FREQ     10000
#define PWM_RES      8
#define L_RPWM_CH    0
#define L_LPWM_CH    1
#define R_RPWM_CH    2
#define R_LPWM_CH    3

// ── Tuning ─────────────────────────────────────────────
#define DRIVE_SPEED   200
#define SPIN_SPEED    180
#define SPIN_360_MS   2000

// ═══════════════════════════════════════════════════════
//  BLE PRINT HELPERS  (drop-in Serial replacement)
// ═══════════════════════════════════════════════════════

void blePrint(const char* msg) {
    Serial.print(msg);
    if (bleConnected && txChar) {
        txChar->setValue((uint8_t*)msg, strlen(msg));
        txChar->notify();
        delay(10);   // small gap so BLE stack can flush
    }
}

void blePrintln(const char* msg) {
    String s = String(msg) + "\r\n";  // \r\n for terminal apps
    Serial.println(msg);
    if (bleConnected && txChar) {
        txChar->setValue((uint8_t*)s.c_str(), s.length());
        txChar->notify();
        delay(10);
    }
}

// ═══════════════════════════════════════════════════════
//  BLE SERVER CALLBACKS
// ═══════════════════════════════════════════════════════

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* server) override {
        bleConnected = true;
        Serial.println("[BLE] Client connected");
    }
    void onDisconnect(BLEServer* server) override {
        bleConnected = false;
        Serial.println("[BLE] Client disconnected — restarting advertising");
        BLEDevice::startAdvertising();  // re-advertise so you can reconnect
    }
};

// ── Optional: receive commands from phone ──────────────
class MyRxCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* rxChar) override {
        String rxValue = rxChar->getValue().c_str();
        if (rxValue.length() > 0) {
            Serial.print("[BLE RX] ");
            Serial.println(rxValue);
            // Command handling lives here — see loop() below
        }
    }
};

// ═══════════════════════════════════════════════════════
//  BLE INIT
// ═══════════════════════════════════════════════════════

void bleInit() {
    BLEDevice::init("ESP32-S3-Motor");

    bleServer = BLEDevice::createServer();
    bleServer->setCallbacks(new MyServerCallbacks());

    BLEService* service = bleServer->createService(NUS_SERVICE_UUID);

    txChar = service->createCharacteristic(NUS_TX_UUID, BLECharacteristic::PROPERTY_NOTIFY);
    txChar->addDescriptor(new BLE2902());

    BLECharacteristic* rxChar = service->createCharacteristic(
        NUS_RX_UUID,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
    );
    rxChar->setCallbacks(new MyRxCallbacks());

    service->start();

    BLEAdvertising* adv = BLEDevice::getAdvertising();
    adv->addServiceUUID(NUS_SERVICE_UUID);
    adv->setScanResponse(true);

    // ── Add these two lines ────────────────────────────
    BLEAdvertisementData scanResponse;
    scanResponse.setName("ESP32-S3-Motor");   // forces name into scan response
    adv->setScanResponseData(scanResponse);
    // ──────────────────────────────────────────────────

    BLEDevice::startAdvertising();
    Serial.println("[BLE] Advertising as 'ESP32-S3-Motor'");
}

// ═══════════════════════════════════════════════════════
//  LOW-LEVEL MOTOR DRIVERS
// ═══════════════════════════════════════════════════════

void motorsEnable() {
    digitalWrite(L_R_EN_PIN, HIGH); digitalWrite(L_L_EN_PIN, HIGH);
    digitalWrite(R_R_EN_PIN, HIGH); digitalWrite(R_L_EN_PIN, HIGH);
    blePrintln("[EN] Motors ENABLED");
}

void motorsDisable() {
    ledcWrite(L_RPWM_CH, 0); ledcWrite(L_LPWM_CH, 0);
    ledcWrite(R_RPWM_CH, 0); ledcWrite(R_LPWM_CH, 0);
    digitalWrite(L_R_EN_PIN, LOW); digitalWrite(L_L_EN_PIN, LOW);
    digitalWrite(R_R_EN_PIN, LOW); digitalWrite(R_L_EN_PIN, LOW);
    blePrintln("[EN] Motors DISABLED");
}

inline void setLeft(uint8_t fwd, uint8_t rev) {
    ledcWrite(L_RPWM_CH, fwd);
    ledcWrite(L_LPWM_CH, rev);
}

inline void setRight(uint8_t fwd, uint8_t rev) {
    ledcWrite(R_RPWM_CH, fwd);
    ledcWrite(R_LPWM_CH, rev);
}

// ═══════════════════════════════════════════════════════
//  MOTION PRIMITIVES
// ═══════════════════════════════════════════════════════

void driveForward(int speed) { setLeft(speed, 0);  setRight(speed, 0); }
void driveReverse(int speed) { setLeft(0, speed);  setRight(0, speed); }
void coast()                 { setLeft(0, 0);      setRight(0, 0);     }
void brake()                 { setLeft(255, 255);  setRight(255, 255); }
void spinCW(int speed)       { setLeft(speed, 0);  setRight(0, speed); }
void spinCCW(int speed)      { setLeft(0, speed);  setRight(speed, 0); }

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

// ═══════════════════════════════════════════════════════
//  SETUP
// ═══════════════════════════════════════════════════════

void setup() {
    Serial.begin(115200);
    delay(500);

    bleInit();   // start BLE advertising immediately

    // EN pins
    pinMode(L_R_EN_PIN, OUTPUT); digitalWrite(L_R_EN_PIN, LOW);
    pinMode(L_L_EN_PIN, OUTPUT); digitalWrite(L_L_EN_PIN, LOW);
    pinMode(R_R_EN_PIN, OUTPUT); digitalWrite(R_R_EN_PIN, LOW);
    pinMode(R_L_EN_PIN, OUTPUT); digitalWrite(R_L_EN_PIN, LOW);

    // LEDC channels
    ledcSetup(L_RPWM_CH, PWM_FREQ, PWM_RES);
    ledcSetup(L_LPWM_CH, PWM_FREQ, PWM_RES);
    ledcSetup(R_RPWM_CH, PWM_FREQ, PWM_RES);
    ledcSetup(R_LPWM_CH, PWM_FREQ, PWM_RES);

    ledcAttachPin(L_RPWM_PIN, L_RPWM_CH);
    ledcAttachPin(L_LPWM_PIN, L_LPWM_CH);
    ledcAttachPin(R_RPWM_PIN, R_RPWM_CH);
    ledcAttachPin(R_LPWM_PIN, R_LPWM_CH);

    coast();
    blePrintln("System ready. Connect via BLE to see logs.");
    blePrintln("Enabling motors in 2 seconds...");
    delay(2000);

    motorsEnable();
    delay(500);
    blePrintln("Starting sequence...");
}

// ═══════════════════════════════════════════════════════
//  MAIN SEQUENCE
// ═══════════════════════════════════════════════════════

void loop() {
    blePrintln("1.  Drive FORWARD 2s");
    driveForward(DRIVE_SPEED);
    delay(2000);

    blePrintln("2.  Slow STOP (forward) 2s");
    slowStop(DRIVE_SPEED, true, 2000);

    blePrintln("3.  COAST 2s");
    coast(); delay(2000);

    blePrintln("4.  Drive REVERSE 2s");
    driveReverse(DRIVE_SPEED);
    delay(2000);

    blePrintln("5.  Slow STOP (reverse) 2s");
    slowStop(DRIVE_SPEED, false, 2000);

    blePrintln("6.  COAST 2s");
    coast(); delay(2000);

    blePrintln("7.  Drive FORWARD 2s");
    driveForward(DRIVE_SPEED);
    delay(2000);

    blePrintln("8.  HARD BRAKE");
    brake(); delay(800); coast(); delay(1200);

    blePrintln("9.  Drive REVERSE 2s");
    driveReverse(DRIVE_SPEED);
    delay(2000);

    blePrintln("10. HARD BRAKE");
    brake(); delay(800); coast(); delay(1200);

    blePrintln("11. Spin 360 CW");
    spinCW(SPIN_SPEED); delay(SPIN_360_MS); coast(); delay(1000);

    blePrintln("12. Spin 360 CCW");
    spinCCW(SPIN_SPEED); delay(SPIN_360_MS); coast(); delay(2000);

    blePrintln("Sequence complete. Repeating...\n");
}