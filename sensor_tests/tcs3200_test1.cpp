#include <Arduino.h>

#define S0_PIN   4
#define S1_PIN   5
#define S2_PIN   7
#define S3_PIN   15
#define OUT_PIN  16
#define LED_PIN  6

void setup() {
    Serial.begin(115200);

    pinMode(S0_PIN,  OUTPUT);
    pinMode(S1_PIN,  OUTPUT);
    pinMode(S2_PIN,  OUTPUT);
    pinMode(S3_PIN,  OUTPUT);
    pinMode(OUT_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);

    // Set 20% output scaling
    digitalWrite(S0_PIN, HIGH);
    digitalWrite(S1_PIN, LOW);

    // Turn on white LEDs
    digitalWrite(LED_PIN, HIGH);

    Serial.println("TCS3200 Test Started");
}

int readFrequency(int s2State, int s3State) {
    digitalWrite(S2_PIN, s2State);
    digitalWrite(S3_PIN, s3State);
    delay(10);  // settle time
    // Measure pulse width in microseconds
    // Lower frequency = higher number = more light
    return pulseIn(OUT_PIN, LOW, 100000);
}

void loop() {
    // Read RED   (S2=LOW,  S3=LOW)
    int red   = readFrequency(LOW,  LOW);

    // Read GREEN (S2=HIGH, S3=HIGH)
    int green = readFrequency(HIGH, HIGH);

    // Read BLUE  (S2=LOW,  S3=HIGH)
    int blue  = readFrequency(LOW,  HIGH);

    // Read CLEAR (S2=HIGH, S3=LOW)
    int clear = readFrequency(HIGH, LOW);

    Serial.print("R="); Serial.print(red);
    Serial.print("  G="); Serial.print(green);
    Serial.print("  B="); Serial.print(blue);
    Serial.print("  C="); Serial.println(clear);

    delay(200);
}