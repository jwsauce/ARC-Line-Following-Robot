#include <Arduino.h>
// Define the number of sensors
const int numSensors = 5;

// Define the pins in an array based on your pinout
const int irPins[numSensors] = {1, 2, 42, 41, 40};

// Array to store the sensor values
int sensorValues[numSensors];

void setup() {
  // Start Serial communication for debugging
  Serial.begin(115200);

  // Initialize all IR pins as INPUT
  for (int i = 0; i < numSensors; i++) {
    pinMode(irPins[i], INPUT);
  }

  Serial.println("IR Sensor Array Initialized...");
}

void loop() {
  // Read each sensor and store the value
  for (int i = 0; i < numSensors; i++) {
    sensorValues[i] = digitalRead(irPins[i]);
  }

  // Print values to Serial Monitor: [S1 S2 S3 S4 S5]
  Serial.print("Sensor Readings: ");
  for (int i = 0; i < numSensors; i++) {
    Serial.print(sensorValues[i]);
    Serial.print(" ");
  }
  Serial.println(); // New line for next reading

  delay(100); // Small delay for stability
}