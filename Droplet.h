// This source file contains the code for creating the water droplet character that is being called during watering animation.

#ifndef DROPLET_H
#define DROPLET_H

#include <LiquidCrystal_I2C.h>

// --- Define the droplet character ---
byte droplet[8] = {
  0b00100,
  0b00100,
  0b01010,
  0b01010,
  0b10001,
  0b10001,
  0b01110,
  0b00000
};

// --- Function to create droplet character ---
void createDroplet(LiquidCrystal_I2C &lcd) {
  lcd.createChar(0, droplet);
}

#endif
