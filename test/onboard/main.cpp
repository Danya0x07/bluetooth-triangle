#include <Arduino.h>
#include "config.h"

uint8_t pins[3] = {RED_STRIP_PIN, GREEN_STRIP_PIN, BLUE_STRIP_PIN};

void setup()
{
    for (uint8_t i = 0; i < 3; i++) {
        pinMode(pins[i], OUTPUT);
    }
}

void loop()
{
    for (uint8_t i = 0; i < 3; i++) {
        analogWrite(pins[i], 0);
        delay(300);
    }
     for (uint8_t i = 0; i < 3; i++) {
        analogWrite(pins[i], 0xFF);
        delay(300);
    }
}