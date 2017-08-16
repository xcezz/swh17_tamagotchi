/*

  by Christian Winkler
  and Nina Hösl

 ********** TAMAGUINO ***********
   Tamagotchi clone for Arduino
 ********************************

*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

static const unsigned long REFRESH_INTERVAL = 50; // ms
static unsigned long previousMillis = 0;

static const uint8_t PROGMEM
happy_bmp[] =
{ B00000000,
  B00000000,
  B00000000,
  B00000000,
  B01000010,
  B00100100,
  B00011000,
  B00000000
},
tired_bmp[] =
{ B00111100,
  B01000010,
  B10100101,
  B10000001,
  B10111101,
  B10000001,
  B01000010,
  B00111100
},
angry_bmp[] =
{ B00111100,
  B01000010,
  B10100101,
  B10000001,
  B10011001,
  B10100101,
  B01000010,
  B00111100
};

/* ------- PET STATS ------- */

float hunger = 100;
float happiness = 100;
float energy = 100;
float health = 100;
float age = 0;
// float weight = 1;

float scalingFactor = 1;
bool dead = false;
bool sleeping = false;


void setup() {
  Serial.begin(9600);

}

void loop() {

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= REFRESH_INTERVAL) {
    previousMillis = currentMillis;

    /* ------- MODIFY PET STATS ------- */
    if (!dead) {
      if (sleeping) {
        hunger -= 0.00005 * scalingFactor;
        if (happiness - 0.0001 > 0) {
          happiness -= 0.0001 * scalingFactor;
        }
        energy += 0.01 * scalingFactor;
      } else {
        hunger -= 0.00025 * scalingFactor;
        if (happiness - 0.0002 > 0) {
          happiness -= 0.0002 * scalingFactor;
        }
        energy -= 0.0001 * scalingFactor;
      }
      age += 0.000025 * scalingFactor;
      health = (hunger + happiness + energy) / 3;
    }
  }

  // STATUS: threshold exceeded
  if ((hunger > 19.99975 && hunger < 20.00025) || (happiness > 19.9998 && happiness < 20.0002) || (health > 18 && health < 20)) {
    Serial.println("YOUR PET IS CRYING");
  }

  // STATUS: good
  if (hunger > 20 && happiness > 20 && health > 20) {

  }

  // STATUS: critical
  if (hunger <= 20 || happiness <= 20 || health <= 20) {
    Serial.println("CRITICAL STATUS");
  }

  // STATUS: dead
  if (hunger <= 0 || health <= 0 || happiness <= 0) {
    dead = true;
  }
}
