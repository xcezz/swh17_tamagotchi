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

int photocellReading;

/* ------- RFID TAGS ------- */

int tag_yellow[14] = {2, 48, 56, 48, 48, 48, 65, 56, 57, 55, 51, 70, 56, 3};
int tag_red[14] = {2, 48, 55, 48, 48, 69, 51, 52, 48, 49, 67, 66, 56, 3};
int newtag[14] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // used for read comparisons

int rfidData = 0;
int rfidCheck = -1;

/* ------- PET STATS ------- */
float scalingFactor = 1;

float hunger = 100;
float happiness = 100;
float energy = 100;
float health = 100;
float age = 0;

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

      // Weighting the pet stats
      if (hunger <= 30 || happiness <= 30 || energy <= 30) {
        health = min(hunger, min(happiness, energy));
      } else {
        health = (hunger + happiness + energy) / 3;
      }
    }
  }

  // STATUS: threshold exceeded
  if ((hunger > 29.99975 && hunger < 30.00025) || (happiness > 29.9998 && happiness < 30.0002) || (health > 28 && health < 30) || (energy > 29.9998 && energy < 30.0002)) {
    Serial.println("YOUR PET IS CRYING");
  }

  // STATUS: good
  if (hunger > 20 && happiness > 20 && health > 20 && energy > 20) {

  }

  // STATUS: critical
  if (hunger <= 20 || happiness <= 20 || health <= 20 || energy <= 20) {
    Serial.println("CRITICAL STATUS");
  }

  // STATUS: dead
  if (hunger <= 0 || health <= 0 || happiness <= 0 || energy <= 0) {
    dead = true;
    delay(5000);
    softReset();
  }

  if (photocellReading <= 70) {
    sleeping = true;
    // Augen + Sound = Schlafen
  } else {
    sleeping = false;
    // Reset Augen + Sound?
  }

  readTags();
}

boolean compareTag(int aa[14], int bb[14]) {
  boolean ff = false;
  int fg = 0;

  for (int cc = 0 ; cc < 14 ; cc++) {
    if (aa[cc] == bb[cc]) {
      fg++;
    }
  }
  if (fg == 14) {
    ff = true;
  }
  return ff;
}

// Compares each tag against the tag just read
void checkTags() {

  // this variable helps decision-making,
  // if it is 1 we have a match, zero is a read but no match,
  // -1 is no read attempt made
  rfidCheck = 0;

  if (compareTag(newtag, tag_red) == true || compareTag(newtag, tag_yellow) == true) {
    rfidCheck++;
  }
}

void readTags() {
  rfidCheck = -1;

  if (Serial1.available() > 0) {
    delay(100);

    for (int z = 0 ; z < 14 ; z++) {
      rfidData = Serial1.read();
      newtag[z] = rfidData;
    }
    Serial1.flush(); // stops multiple reads

    // do the tags match up?
    checkTags();
  }

  // now do something based on tag type
  // > 0  -> match
  // = 0  -> no match
  if (rfidCheck > 0) {
    Serial.println("RFID ACCEPTED");
    hunger += 5;

    rfidCheck = -1;
  }
  else if (rfidCheck == 0) {
    Serial.println("RFID REJECTED");

    rfidCheck = -1;
  }
}

// Restarts the sketch
// http://www.xappsoftware.com/wordpress/2013/06/24/three-ways-to-reset-an-arduino-board-by-code/
void softReset() {
  asm volatile ("  jmp 0");
}
