#include <Arduino.h>

// Define the button pin
#define BUTTON_PIN 16

// Variables to track button state
bool lastButtonState = HIGH; // Default state for PULLUP is HIGH

void setup() {
  // ESP32-S3 standard baud rate
  Serial.begin(115200);

  // Initialize the button with internal pull-up resistor
  // This means the pin is HIGH by default and goes LOW when pressed
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println("System Ready. Waiting for button press...");
}

void loop() {
  // Read the current state of the button
  bool currentButtonState = digitalRead(BUTTON_PIN);

  // Check if the button state has changed (from HIGH to LOW)
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    // A press is detected
    Serial.println(">>> [SIGNAL] Button Pressed!");
    
    // Small delay to "debounce" (prevent flickering signals)
    delay(50); 
  }

  // Save the current state for the next loop
  lastButtonState = currentButtonState;
}