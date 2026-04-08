#include "ColorSensor.h"

void initColorSensor() {
    pinMode(S0_PIN,  OUTPUT);
    pinMode(S1_PIN,  OUTPUT);
    pinMode(S2_PIN,  OUTPUT);
    pinMode(S3_PIN,  OUTPUT);
    pinMode(OUT_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);

    digitalWrite(S0_PIN, HIGH);
    digitalWrite(S1_PIN, LOW);
    digitalWrite(LED_PIN, HIGH);
}

int readFrequency(int s2State, int s3State) {
    digitalWrite(S2_PIN, s2State);
    digitalWrite(S3_PIN, s3State);
    delay(10);
    return pulseIn(OUT_PIN, LOW, 100000);
}

String detectColour(int r, int g, int b) {
    if (r > 2300 && g > 2200 && b > 2200) return "BLACK";
    if (r < 1550 && g < 1100 && b < 1200) return "WHITE";
    if (r < 1550 && g < 1400 && b > 950)  return "YELLOW";

    int minVal = min(r, min(g, b));
    if (minVal == r) return "RED";
    if (minVal == b) return "BLUE";
    if (minVal == g) return "GREEN";

    return "UNKNOWN";
}

bool confirmWhite(int samples) {
    long sumR = 0, sumG = 0, sumB = 0;
    for (int i = 0; i < samples; i++) {
        sumR += readFrequency(LOW,  LOW);
        sumG += readFrequency(HIGH, HIGH);
        sumB += readFrequency(LOW,  HIGH);
        delay(30);
    }
    int avgR = sumR / samples;
    int avgG = sumG / samples;
    int avgB = sumB / samples;
    return (avgR < 1550 && avgG < 1100 && avgB < 1200);
}

String scanColourStable(int samples) {
    int r=0, g=0, b=0, y=0, blk=0, white=0, unk=0;

    for (int i = 0; i < samples; i++) {
        int red   = readFrequency(LOW,  LOW);
        int green = readFrequency(HIGH, HIGH);
        int blue  = readFrequency(LOW,  HIGH);

        String c = detectColour(red, green, blue);
        if      (c == "RED")    r++;
        else if (c == "GREEN")  g++;
        else if (c == "BLUE")   b++;
        else if (c == "YELLOW") y++;
        else if (c == "BLACK")  blk++;
        else if (c == "WHITE")  white++;
        else                    unk++;
        delay(30);
    }

    int mx = max({r, g, b, y, blk, white, unk});

    if (white == mx) {
        if (confirmWhite(10)) return "WHITE";
        else {
            white = -1;
            mx = max({r, g, b, y, blk, unk});
        }
    }

    if (blk == mx) return "BLACK";
    if (r   == mx) return "RED";
    if (g   == mx) return "GREEN";
    if (b   == mx) return "BLUE";
    if (y   == mx) return "YELLOW";
    return "UNKNOWN";
}