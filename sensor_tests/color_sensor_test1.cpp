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

    digitalWrite(S0_PIN, HIGH);
    digitalWrite(S1_PIN, LOW);
    digitalWrite(LED_PIN, HIGH);

    Serial.println("TCS3200 Test Started");
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

// NEW: Confirm white by averaging 10 samples and re-checking thresholds
bool confirmWhite(int samples = 10) {
    long sumR = 0, sumG = 0, sumB = 0;

    Serial.println("  [White Confirmation] Taking average samples...");
    for (int i = 0; i < samples; i++) {
        sumR += readFrequency(LOW,  LOW);
        sumG += readFrequency(HIGH, HIGH);
        sumB += readFrequency(LOW,  HIGH);
        delay(30);
    }

    int avgR = sumR / samples;
    int avgG = sumG / samples;
    int avgB = sumB / samples;

    Serial.printf("  [White Confirmation] Avg -> R:%d  G:%d  B:%d\n", avgR, avgG, avgB);

    // Re-apply white threshold on averaged values
    return (avgR < 1550 && avgG < 1100 && avgB < 1200);
}

String scanColourStable(int samples = 10) {
    int r=0, g=0, b=0, y=0, blk=0, white=0, unk=0;

    for (int i = 0; i < samples; i++) {
        int red   = readFrequency(LOW,  LOW);
        int green = readFrequency(HIGH, HIGH);
        int blue  = readFrequency(LOW,  HIGH);

        Serial.printf("  Sample %d -> R:%d  G:%d  B:%d\n", i+1, red, green, blue);

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

    // If white wins the vote, do an extra confirmation pass
    if (white == mx) {
        Serial.println("  White detected by voting — running confirmation...");
        if (confirmWhite(10)) {
            Serial.println("  White CONFIRMED.");
            return "WHITE";
        } else {
            Serial.println("  White NOT confirmed — rechecking runner-up...");
            // Remove white from contention and pick next winner
            white = -1;
            mx = max({r, g, b, y, blk, unk});
            if (blk == mx) return "BLACK";
            if (r   == mx) return "RED";
            if (g   == mx) return "GREEN";
            if (b   == mx) return "BLUE";
            if (y   == mx) return "YELLOW";
            return "UNKNOWN";
        }
    }

    if (blk   == mx) return "BLACK";
    if (r     == mx) return "RED";
    if (g     == mx) return "GREEN";
    if (b     == mx) return "BLUE";
    if (y     == mx) return "YELLOW";
    return "UNKNOWN";
}

void loop() {
    Serial.println("--- Scanning ---");
    String colour = scanColourStable(10);
    Serial.print(">>> Detected colour: ");
    Serial.println(colour);
    Serial.println();
    delay(1000);
}
