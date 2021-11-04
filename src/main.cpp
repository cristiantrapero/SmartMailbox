#include <Arduino.h>
#include "Keypad.h"

// Keypad size
const byte ROWS = 4;
const byte COLS = 4;

// Keypad keys distribution
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

// Keypad pinout
byte rowPins[ROWS] = {13, 12, 14, 27}; 
byte colPins[COLS] = {26, 25, 33, 32};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
  Serial.begin(9600);
}

void loop() {
  char pressedKey = keypad.getKey();

  if (pressedKey){
    Serial.println(pressedKey);
  }
}