/*
  by Christian Winkler
  and Nina Hoesl

 ********** TAMAGUINO ***********
   Tamagotchi clone for Arduino
 ********************************
*/

#include "Arduino.h"
#include "SoftwareSerial.h"
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include "DFRobotDFPlayerMini.h"

#include "anim.h"

#ifdef __AVR__
#include <avr/power.h>
#endif


/* ------- DFPLAYER MINI ------- */
SoftwareSerial dfPlayerSerial(10, 11); //RX, TX
DFRobotDFPlayerMini dfPlayer;

// Sound IDs
int soundStart = 1;
int soundCry = 2;
int soundLaugh = 3;
int soundEat = 4;
int soundBurp = 5;
int soundSnore = 6;
int soundLost = 7;
int soundGameOver = 8;
int soundNeigh = 9;
int soundGasp = 10;

/* ------- LED STRIPS ------- */
Adafruit_NeoPixel backStrip = Adafruit_NeoPixel(5, 7, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel hornStrip = Adafruit_NeoPixel(11, 8, NEO_GRB + NEO_KHZ800);

/* ------- LED MATRICES ------- */
Adafruit_8x8matrix leftEye = Adafruit_8x8matrix();
Adafruit_8x8matrix rightEye = Adafruit_8x8matrix();
Adafruit_8x8matrix heart = Adafruit_8x8matrix();

int modeLength = 0;
int blinkDelay = 100;
int blinkChance = 10;
int blinkChanceResult;
int animSelection = 0;

/* ------- FORCE SENSITIVE RESISTORS ------- */
int fsrFrontRight = A0;  // front right (A0)
int fsrFrontLeft = A1;   // front left (A1)
int fsrBackRight = A2;   // back right (A2)
int fsrBackLeft = A3;    // back left (A3)

/* ------- RFID TAGS ------- */
int tag_yellow[14] = {2, 48, 56, 48, 48, 48, 65, 56, 57, 55, 51, 70, 56, 3};
int tag_red[14] = {2, 48, 55, 48, 48, 69, 51, 52, 48, 49, 67, 66, 56, 3};
int newtag[14] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // used for read comparisons

int rfidData = 0;
int rfidCheck = -1;

/* ------- VIBRATION MOTOR ------- */
int vibrationMotorPin = 6;

/* ------- COPPER PIN ------- */
int copperPin = 4;

/* ------- PHOTOCELL ------- */
int photocellPin = A4;

/* ------- PET STATS ------- */
float scalingFactor = 10;

float hunger = 100;
float happiness = 100;
float energy = 100;
float health = 100;
float age = 0;

bool dead = false;
bool sleeping = false;

static const unsigned long REFRESH_INTERVAL_STATS = 50; // 0.05s
static const unsigned long REFRESH_INTERVAL_LED = 5000; // 5s
static const unsigned long REFRESH_INTERVAL_SOUND = 20000; // 20s
static const unsigned long REFRESH_INTERVAL_VIBRATION = 1500; // 1.5s
static const unsigned long REFRESH_INTERVAL_DEBUG = 10000; // 10s
static unsigned long previousMillisStats = 0;
static unsigned long previousMillisLed = 0;
static unsigned long previousMillisSound = 0;
static unsigned long previousMillisVibration = 0;
static unsigned long previousMillisDebug = 0;
unsigned long currentMillis;

boolean resetBackLed = false;

void setup() {
  // Initialize DFPlayer Mini
  dfPlayerSerial.begin(9600);
  dfPlayer.begin(dfPlayerSerial);

  Serial1.begin(9600);
  Serial.begin(9600);

  // Initialize all pixel strips to 'off'
  backStrip.begin();
  backStrip.show();
  hornStrip.begin();
  hornStrip.show();

  // Set pins
  pinMode(copperPin, INPUT);
  pinMode(vibrationMotorPin, OUTPUT);

  // Pass the led-matrix addresses
  leftEye.begin(0x70);
  rightEye.begin(0x71);
  heart.begin(0x72);

  // Set volume value (0 - 30)
  dfPlayer.volume(25);
  dfPlayer.play(soundStart);
}

void loop() {
  currentMillis = millis();

  if (currentMillis - previousMillisStats >= REFRESH_INTERVAL_STATS) {
    previousMillisStats = currentMillis;

    /* ------- MODIFY PET STATS ------- */
    if (!dead) {
      if (sleeping) {
        hunger -= 0.0005 * scalingFactor;
        if (happiness - 0.001 > 0) {
          happiness -= 0.001 * scalingFactor;
        }
        energy += 0.1 * scalingFactor;
      } else {
        hunger -= 0.0025 * scalingFactor;
        if (happiness - 0.002 > 0) {
          happiness -= 0.002 * scalingFactor;
        }
        energy -= 0.001 * scalingFactor;
      }
      age += 0.00025 * scalingFactor;

      // Weighting the pet stats
      if (hunger <= 30 || happiness <= 30 || energy <= 30) {
        health = min(hunger, min(happiness, energy));
      } else {
        health = (hunger + happiness + energy) / 3;
      }
    }
  }

  if (currentMillis - previousMillisDebug >= REFRESH_INTERVAL_DEBUG) {
    previousMillisDebug = currentMillis;

    debugLog();
  }

  // STATUS: threshold exceeded
  if ((hunger > 29.99975 && hunger < 30.00025) ||
      (happiness > 29.9998 && happiness < 30.0002) ||
      (health > 28 && health < 30) ||
      (energy > 29.9998 && energy < 30.0002)) {
    Serial.println("YOUR PET IS CRYING");
    dfPlayer.play(soundCry);
  }

  // STATUS: good
  if (hunger > 20 && happiness > 20 && health > 20 && energy > 20) {
    animationTwinkle();

    // Play random sound
    if (currentMillis - previousMillisSound >= REFRESH_INTERVAL_SOUND) {
      previousMillisSound = currentMillis;

      playRandomSound();
    }
  }

  // STATUS: critical
  if (hunger <= 20 || happiness <= 20 || health <= 20 || energy <= 20) {
    Serial.println("CRITICAL STATUS");
    animationAngry();
  }

  //STATUS: hungry
  if (hunger <= 20) {
    analogWrite(vibrationMotorPin, 110);

    if (currentMillis - previousMillisVibration >= REFRESH_INTERVAL_VIBRATION) {
      previousMillisVibration = currentMillis;

      analogWrite(vibrationMotorPin, 0);
    }
  } else {
    analogWrite(vibrationMotorPin, 0);
  }

  // STATUS: dead
  if (hunger <= 0 || health <= 0 || happiness <= 0 || energy <= 0) {
    dead = true;
    animationDead();
    animationSad();
    dfPlayer.play(soundLost);
    delay(10000);
    softReset();
  }

  // Cuddling
  if (digitalRead(copperPin) == 1) {
    happiness += 5;
    dfPlayer.play(soundLaugh);
  }

  // Check if the pet is sleeping
  if (analogRead(photocellPin) <= 40) {
    sleeping = true;
  } else {
    sleeping = false;
  }

  if (sleeping) {
    animationSleepy();
    dfPlayer.play(soundSnore);
    delay(2000);
  }

  // Check hitting on paws
  if (analogRead(fsrFrontRight) > 500) {
    for (uint16_t i = 0; i < getPetStatus(health); i++) {
      backStrip.setPixelColor(i, 255, 0, 0);
      backStrip.show();
      resetBackStrip();
    }
  }

  if (analogRead(fsrFrontLeft) > 500) {
    for (uint16_t i = 0; i < getPetStatus(happiness); i++) {
      backStrip.setPixelColor(i, 255, 255, 0);
      backStrip.show();
      resetBackStrip();
    }
  }

  if (analogRead(fsrBackRight) > 500) {
    for (uint16_t i = 0; i < getPetStatus(energy); i++) {
      backStrip.setPixelColor(i, 0, 0, 255);
      backStrip.show();
      resetBackStrip();
    }
  }

  if (analogRead(fsrBackLeft) > 500) {
    for (uint16_t i = 0; i < getPetStatus(hunger); i++) {
      backStrip.setPixelColor(i, 0, 255, 0);
      backStrip.show();
      resetBackStrip();
    }
  }

  // Resets the LED strip on the back
  if (resetBackLed) {
    if (currentMillis - previousMillisLed >= REFRESH_INTERVAL_LED) {
      resetBackLed = false;
      previousMillisLed = currentMillis;

      for (uint16_t i = 0; i < 5; i++) {
        backStrip.setPixelColor(i, 0, 0, 0);
        backStrip.show();
      }
    }
  }

  readTags();
  animateHorn();
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
    delay(100);
    Serial.println("RFID ACCEPTED");
    hunger += 5;
    dfPlayer.play(soundEat);

    rfidCheck = -1;
  }
  else if (rfidCheck == 0) {
    Serial.println("RFID REJECTED");

    rfidCheck = -1;
  }
}

float getPetStatus(float value) {
  value = map(value, 0, 100, 1, 5);
  return value;
}

// Resets the LED Strip
void resetBackStrip() {
  if (!resetBackLed) {
    resetBackLed = true;
    previousMillisLed = currentMillis;
  }
}

// Animate the horn with rainbow colors
// Taken from Adafruit NeoPixel Examples (Strandtest)
void animateHorn() {
  uint16_t i, j;

  for (j = 0; j < 256 * 5; j++) {
    for (i = 3; i < hornStrip.numPixels(); i++) {
      hornStrip.setPixelColor(i, Wheel(((i * 256 / hornStrip.numPixels()) + j) & 255));
    }
    hornStrip.show();
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return hornStrip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return hornStrip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return hornStrip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void eyeUpdate() {
  leftEye.writeDisplay();
  rightEye.writeDisplay();
  leftEye.clear();
  rightEye.clear();
  blinkAnim();
  animationHeart();

}

// Slight chance of eye blinking
void blinkAnim() {
  blinkChanceResult = random(blinkChance);

  if (blinkChanceResult > 8) {
    leftEye.drawBitmap(0, 0, eyesclosed_bmp, 8, 8, LED_ON);
    leftEye.writeDisplay();
    leftEye.clear();
    rightEye.drawBitmap(0, 0, eyesclosed_bmp, 8, 8, LED_ON);
    rightEye.writeDisplay();
    rightEye.clear();
    delay(blinkDelay);
  }
}

void animationTwinkle() {
  animSelection = random(9);
  if (animSelection == 0) {
    rightEye.drawBitmap(0, 0, eyes1_bmp, 8, 8, LED_ON);
    leftEye.drawBitmap(0, 0, eyes1_bmp, 8, 8, LED_ON);
  }
  if (animSelection == 1) {
    rightEye.drawBitmap(0, 0, eyes2_bmp, 8, 8, LED_ON);
    leftEye.drawBitmap(0, 0, eyes2_bmp, 8, 8, LED_ON);
  }
  if (animSelection == 2) {
    rightEye.drawBitmap(0, 0, eyes3_bmp, 8, 8, LED_ON);
    leftEye.drawBitmap(0, 0, eyes3_bmp, 8, 8, LED_ON);
  }
  if (animSelection == 3) {
    rightEye.drawBitmap(0, 0, eyes4_bmp, 8, 8, LED_ON);
    leftEye.drawBitmap(0, 0, eyes4_bmp, 8, 8, LED_ON);
  }
  if (animSelection == 4) {
    rightEye.drawBitmap(0, 0, eyes5_bmp, 8, 8, LED_ON);
    leftEye.drawBitmap(0, 0, eyes5_bmp, 8, 8, LED_ON);
  }
  if (animSelection == 5) {
    rightEye.drawBitmap(0, 0, eyes6_bmp, 8, 8, LED_ON);
    leftEye.drawBitmap(0, 0, eyes6_bmp, 8, 8, LED_ON);
  }
  if (animSelection == 6) {
    rightEye.drawBitmap(0, 0, eyes7_bmp, 8, 8, LED_ON);
    leftEye.drawBitmap(0, 0, eyes7_bmp, 8, 8, LED_ON);
  }
  if (animSelection == 7) {
    rightEye.drawBitmap(0, 0, eyes8_bmp, 8, 8, LED_ON);
    leftEye.drawBitmap(0, 0, eyes8_bmp, 8, 8, LED_ON);
  }
  if (animSelection == 8) {
    rightEye.drawBitmap(0, 0, eyes9_bmp, 8, 8, LED_ON);
    leftEye.drawBitmap(0, 0, eyes9_bmp, 8, 8, LED_ON);
  }
  eyeUpdate();
}

void animationAngry() {
  animSelection = random(3);
  if (animSelection == 0) {
    rightEye.drawBitmap(0, 0, angry1_right_bmp, 8, 8, LED_ON);
    leftEye.drawBitmap(0, 0, angry1_left_bmp, 8, 8, LED_ON);
  }
  if (animSelection == 1) {
    rightEye.drawBitmap(0, 0, angry2_right_bmp, 8, 8, LED_ON);
    leftEye.drawBitmap(0, 0, angry2_left_bmp, 8, 8, LED_ON);
  }
  if (animSelection == 2) {
    rightEye.drawBitmap(0, 0, angry3_right_bmp, 8, 8, LED_ON);
    leftEye.drawBitmap(0, 0, angry3_left_bmp, 8, 8, LED_ON);
  }
  eyeUpdate();
}

void playRandomSound() {
  animSelection = random(2);
  if (animSelection == 0) {
    dfPlayer.play(soundNeigh);
  }
  if (animSelection == 1) {
    dfPlayer.play(soundGasp);
  }
}

void animationDead() {
  rightEye.drawBitmap(0, 0, dead_bmp, 8, 8, LED_ON);
  leftEye.drawBitmap(0, 0, dead_bmp, 8, 8, LED_ON);
  leftEye.writeDisplay();
  rightEye.writeDisplay();
  leftEye.clear();
  rightEye.clear();
}

void animationSleepy() {
  rightEye.drawBitmap(0, 0, sleepy_bmp, 8, 8, LED_ON);
  leftEye.drawBitmap(0, 0, sleepy_bmp, 8, 8, LED_ON);
  leftEye.writeDisplay();
  rightEye.writeDisplay();
  leftEye.clear();
  rightEye.clear();
}

void animationHeart() {
  blinkChanceResult = random(blinkChance);

  heart.clear();
  heart.drawBitmap(0, 0, heart_big_bmp, 8, 8, LED_ON);
  heart.writeDisplay();

  if (blinkChanceResult >= 5) {
    heart.clear();
    heart.drawBitmap(0, 0, heart_small_bmp, 8, 8, LED_ON);
    heart.writeDisplay();
  }
}

void animationSad() {
  heart.drawBitmap(0, 0, sadface_bmp, 8, 8, LED_ON);
  heart.writeDisplay();
  heart.clear();
}

// Used for debugging stats
void debugLog() {
  Serial.println("HEALTH:");
  Serial.print(health);
  Serial.println();
  Serial.println("HUNGER:");
  Serial.print(hunger);
  Serial.println();
  Serial.println("HAPPINESS:");
  Serial.print(happiness);
  Serial.println();
  Serial.println("ENERGY:");
  Serial.print(energy);
  Serial.println();
  Serial.println("AGE:");
  Serial.print(age);
  Serial.println();
  Serial.println("-------------");
  Serial.println();
}

// Restarts the sketch
void softReset() {
  asm volatile ("  jmp 0");
}
