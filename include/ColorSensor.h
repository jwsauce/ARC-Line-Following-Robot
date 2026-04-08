#ifndef COLOR_SENSOR_H
#define COLOR_SENSOR_H

#include <Arduino.h>

// --- Pin Definitions ---
#define S0_PIN   20
#define S1_PIN   21
#define S2_PIN   8
#define S3_PIN   18 
#define OUT_PIN  3
#define LED_PIN  19

// --- Function Declarations ---
// We declare them here so main.cpp knows they exist
void initColorSensor();
int readFrequency(int s2State, int s3State);
String detectColour(int r, int g, int b);
bool confirmWhite(int samples = 10);
String scanColourStable(int samples = 10);

#endif