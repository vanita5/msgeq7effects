/*
Created by: vanita5 <mail@vanit.as>
 
NeoPixel VU meter

This is a port of the Adafruit Amplitie project.
Based on code for the adjustable sensitivity version of amplitie from:
  https://learn.adafruit.com/led-ampli-tie/the-code
 
 Hardware requirements:
 - Trinket 5V 8MHz
 - Almost any NeoPixel product
 
 Software requirements:
 - Adafruit NeoPixel library
 
 Written by Adafruit Industries.  Distributed under the BSD license.
 This paragraph must be included in any redistribution.
 
 fscale function:
 Floating Point Autoscale Function V0.1
 Written by Paul Badger 2007
 Modified from code by Greg Shakar
 */

#include <Adafruit_NeoPixel.h>

/**
 * Audio config
 */
#define AUDIOIN_PIN      1     // The PIN where the audio source is connected to.
#define SAMPLE_WINDOW    25    // Sample window (ms) for the average volume level
#define PEAK_HANG        24    // Time of pause before the peak dot falls
#define PEAK_FALL        4     // Rate of falling peak dot
#define INPUT_FLOOR      20    // Lower range of analogRead input. Might have to be increased if you use a microphone
#define INPUT_CEILING    800   // Upper range of analogRead input. The lower the value, the more sensitive. Max value: 1023

/**
 * NeoPixel config
 */
#define NEOPIXEL_PIN     0     // NeoPixels are connected at PIN 0
#define NUM_NEOPIXELS    16    // The amount of NeoPixels
#define BRIGHTNESS       10    // 0-150. Pixel brightness. NeoPixels (RGBW) can be really(!) bright, so I set this low. Increase if needed.
#define DIRECTION        0     // Change the pixel direction. 0 = left (default); 1 = right
#define BLINK_W          1     // ONLY FOR RGBW AND GRBW NeoPixels! EXPERIMENTAL
#define BLINK_HANG       12    // Time of pause before blink tolerance falls
#define BLINK_FALL       4     // Rate of falling blink tolerance 

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_NEOPIXELS, NEOPIXEL_PIN, NEO_GRBW + NEO_KHZ800); // This setup is for a NeoPixel GRBW Ring with 16 LEDs

/**
 * Global variables
 */
byte peak = 16;           // Peak level of column; used for falling dots
unsigned int sample;      // Sample data
byte dotCount = 0;        // Frame counter for peak dot
byte dotHangCount = 0;    // Frame counter for holding peak dot

byte blinkTolerance = 16; // Peak tolerance level for blinking
byte blinkCount = 0;      // Frame counter for blink value
byte blinkHangCount = 0;  // Frame counter for keepink blink tolerance up

unsigned int bl = 0;      // White blink value
unsigned int last_c = 0;  // Last c value

float fscale(float originalMin, float originalMax, float newBegin, float newEnd, float inputValue, float curve);


void setup() {
  pixels.begin();
  pixels.setBrightness(BRIGHTNESS);

  // clear
  pixels.show();
}

void loop() {
  unsigned int c;
  
  //turn all pixels off
  drawLine(0, NUM_NEOPIXELS - 1, pixels.Color(0, 0, 0, bl));

  float peak2peak = readSample();

  //Scale the input logarithmically
  c = fscale(INPUT_FLOOR, INPUT_CEILING, 0, NUM_NEOPIXELS, peak2peak, 2);

  updateBlink(c);

  updatePeak(c);
  
  //if there is any signal - fill pixels
  if (c > 0) drawLine(DIRECTION * NUM_NEOPIXELS,
                      DIRECTION * (NUM_NEOPIXELS - 2 * c + 1) + c - 1,
                      pixels.Color(0, 255, 0, bl));

  pixels.show();
}

/**
 * Update white blink value
 */
void updateBlink(unsigned int c) {
  if (BLINK_W == 0) return;
  if (blinkTolerance > 0) {
    if (blinkHangCount > BLINK_HANG) { //time is up, blink tolerance must fall
      if (++blinkCount >= BLINK_FALL) {
        blinkTolerance--;
        blinkCount = 0;
      }
    } else {
      blinkHangCount++;
    }
  }

  if (c > blinkTolerance) {
    blinkTolerance = c;
    blinkHangCount = 0;
  }

  if ((DIRECTION == 0 && c > 4) || (DIRECTION == 1 && c < 12)) {
    if (c > last_c + 2) {
      bl = 255;
    }
  }
  if (bl < 20) bl = 0;
  else bl -= 50;
  last_c = c;
}

/**
 * Update peak position depending on
 * peak hang time and fall rate
 */
void updatePeak(unsigned int c) {
  if (peak > 0) {
    if (dotHangCount > PEAK_HANG) { //time is up, peak dot must fall
      if (++dotCount >= PEAK_FALL) { //fall rate
        peak--;
        dotCount = 0;
      }
    } else {
      dotHangCount++;
    }
  }

  //new peak
  if (c > peak) {
    peak = c + 1;
    dotHangCount = 0; //reset dot hang counter
  }

  pixels.setPixelColor(DIRECTION * (NUM_NEOPIXELS - 2 * peak + 1) + peak - 1, 255, 0, 0, bl);
}

/**
 * Read a sample from the audio source and
 * return the peak to peak amplitude
 */
float readSample() {
  unsigned int signalMax = 0;
  unsigned int signalMin = 1023;
  unsigned long startMillis = millis();

  while (millis() - startMillis < SAMPLE_WINDOW) {
    sample = analogRead(AUDIOIN_PIN) - INPUT_FLOOR;

    //toss out spurious readings
    if (sample > 1023) continue;

    if (sample > signalMax) signalMax = sample;
    else if (sample < signalMin) signalMin = sample;
  }
  return signalMax - signalMin;
}

/**
 * Used to draw a line between two points of a given color
 */
void drawLine(uint8_t from, uint8_t to, uint32_t c) {
  uint8_t fromTemp;
  if (from > to) {
    fromTemp = from;
    from = to;
    to = fromTemp;
  }
  for(int i = from; i <= to; i++) {
    pixels.setPixelColor(i, c);
  }
}

float fscale(float originalMin, float originalMax, float newBegin, float newEnd, float inputValue, float curve) {
  float OriginalRange = 0;
  float NewRange = 0;
  float zeroRefCurVal = 0;
  float normalizedCurVal = 0;
  float rangedValue = 0;
  boolean invFlag = 0;

  // condition curve parameter
  // limit range
  if (curve > 10) curve = 10;
  if (curve < -10) curve = -10;

  curve = (curve * -.1) ; // - invert and scale - this seems more intuitive - postive numbers give more weight to high end on output 
  curve = pow(10, curve); // convert linear scale into lograthimic exponent for other pow function

  // Check for out of range inputValues
  if (inputValue < originalMin) {
    inputValue = originalMin;
  }
  if (inputValue > originalMax) {
    inputValue = originalMax;
  }

  // Zero Refference the values
  OriginalRange = originalMax - originalMin;

  if (newEnd > newBegin) { 
    NewRange = newEnd - newBegin;
  } else {
    NewRange = newBegin - newEnd; 
    invFlag = 1;
  }

  zeroRefCurVal = inputValue - originalMin;
  normalizedCurVal  =  zeroRefCurVal / OriginalRange;   // normalize to 0 - 1 float

  // Check for originalMin > originalMax  - the math for all other cases i.e. negative numbers seems to work out fine 
  if (originalMin > originalMax) {
    return 0;
  }

  if (invFlag == 0) {
    rangedValue = (pow(normalizedCurVal, curve) * NewRange) + newBegin;
  } else {    // invert the ranges
    rangedValue =  newBegin - (pow(normalizedCurVal, curve) * NewRange); 
  }

  return rangedValue;
}

