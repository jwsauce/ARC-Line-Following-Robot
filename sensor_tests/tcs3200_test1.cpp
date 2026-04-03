#include <Arduino.h>

#define S0_PIN   A5
#define S1_PIN   A4
#define S2_PIN   A3
#define S3_PIN   A2
#define OUT_PIN  A1
#define LED_PIN  A0 

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

/*
Results:

Surface,R,G,B,C,Logic
White,20,18,14,5,All values are very low because white reflects all light.
Red,30,95,72,18,R is the lowest (strongest signal).
Blue,98,46,22,12, B is the lowest (strongest signal)
Yellow,20,27,43,8,Both R and G are low (Red + Green = Yellow).
Green,144,61,64,24, Both G and B are low (gemini say is spectral overlap? idk)
Black,142,131,101,40,All values are high because black absorbs light

*/
