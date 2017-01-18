/*
Created by: vanita5 <mail@vanit.as>
 
MSGEQ7 / RGB LED Strip Effect 1
 
 Hardware requirements:
 - Arduino Uno
 - Almost any NeoPixel product
 - 2x MSGEQ7
 
 Software requirements:
 - Adafruit NeoPixel library
 - MSGEQ7 library <https://github.com/NicoHood/MSGEQ7>
 
 fscale function:
 Floating Point Autoscale Function V0.1
 Written by Paul Badger 2007
 Modified from code by Greg Shakar
 */

#include <Adafruit_NeoPixel.h>
#include <MSGEQ7.h>

#define NEOPIXEL_PIN     6     // NeoPixels are connected at PIN 0
#define NUM_NEOPIXELS    144    // The amount of NeoPixels
#define BRIGHTNESS       100    // 0-150. Pixel brightness. NeoPixels (RGBW) can be really(!) bright, so I set this low. Increase if needed.

#define pinAnalogLeft A0
#define pinAnalogRight A1
#define pinReset 5
#define pinStrobe 4
#define MSGEQ7_INTERVAL ReadsPerSecond(50)
#define MSGEQ7_SMOOTH 100 // Range: 0-255

#define SNAKE_BORDER 240

#define CENTER_WIDTH  2    // Width of the "burning" center; make sure this is divisible by 2
#define CENTER_SPREAD 16    // Width of the pulse. +CENTER_SPREAD on each side (L/R)

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_NEOPIXELS, NEOPIXEL_PIN, NEO_GRBW + NEO_KHZ800); // This setup is for a NeoPixel GRBW Ring with 16 LEDs

CMSGEQ7<MSGEQ7_SMOOTH, pinReset, pinStrobe, pinAnalogLeft> MSGEQ7Left;
CMSGEQ7<MSGEQ7_SMOOTH, pinReset, pinStrobe, pinAnalogRight> MSGEQ7Right;

float fscale(float originalMin, float originalMax, float newBegin, float newEnd, float inputValue, float curve);

uint32_t colors_center[NUM_NEOPIXELS];
uint32_t colors_center_snake[NUM_NEOPIXELS];

uint8_t center_from;
uint8_t center_to;
uint8_t middle;

void setup() {
  middle = floor(NUM_NEOPIXELS / 2) - 1;
  center_from = middle - CENTER_WIDTH / 2;
  center_to = middle + CENTER_WIDTH / 2;
  
  strip.begin();
  strip.setBrightness(BRIGHTNESS);

  MSGEQ7Left.begin();
  MSGEQ7Right.begin();

  Serial.begin(9600);

  for (int i = 0; i < strip.numPixels(); i++) {
    colors_center_snake[i] = strip.Color(0, 0, 0, 0);
  }

  // clear
  strip.show();
}

uint8_t lastBass_L = 0;
uint8_t lastBass_R = 0;

void loop() {
  //clear
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0, 0));
    // REMOVE
    colors_center[i] = strip.Color(0, 0, 0, 0);
  }
  
  bool newReadingLeft = MSGEQ7Left.read();
  bool newReadingRight = MSGEQ7Right.read();

  uint8_t freqL0 = MSGEQ7Left.get(0);  
  uint8_t freqL1 = MSGEQ7Left.get(1);
  uint8_t freqL2 = MSGEQ7Left.get(2);
  uint8_t freqL3 = MSGEQ7Left.get(3);  
  uint8_t freqL4 = MSGEQ7Left.get(4);
  uint8_t freqL5 = MSGEQ7Left.get(5);
  uint8_t freqL6 = MSGEQ7Left.get(6);

  uint8_t freqR0 = MSGEQ7Right.get(0);  
  uint8_t freqR1 = MSGEQ7Right.get(1);
  uint8_t freqR2 = MSGEQ7Right.get(2);
  uint8_t freqR3 = MSGEQ7Right.get(3);  
  uint8_t freqR4 = MSGEQ7Right.get(4);
  uint8_t freqR5 = MSGEQ7Right.get(5);
  uint8_t freqR6 = MSGEQ7Right.get(6);
  
  freqL0 = mapNoise(freqL0);
  freqL1 = mapNoise(freqL1);
  freqL2 = mapNoise(freqL2);
  freqL3 = mapNoise(freqL3);
  freqL4 = mapNoise(freqL4);
  freqL5 = mapNoise(freqL5);
  freqL6 = mapNoise(freqL6);
  
  freqR0 = mapNoise(freqR0);
  freqR1 = mapNoise(freqR1);
  freqR2 = mapNoise(freqR2);
  freqR3 = mapNoise(freqR3);
  freqR4 = mapNoise(freqR4);
  freqR5 = mapNoise(freqR5);
  freqR6 = mapNoise(freqR6);

  /* Bass detection stuff */
  lastBass_L -= 4;
  lastBass_R -= 4;

  uint8_t redBassIncL = 1;
  uint8_t redBassIncR = 1;

  if (freqL0 > 190 && freqL0 > lastBass_L) {
    redBassIncL = 0;
    lastBass_L = freqL0;

    AddSnake(middle - CENTER_SPREAD + 3, middle - CENTER_SPREAD + 5, strip.Color(255, 0, 10, 0));
  }

  if (freqR0 > 180 && freqR0 > lastBass_R) {
    redBassIncR = 0;
    lastBass_R = freqR0;
    
    AddSnake(middle + CENTER_SPREAD - 3, middle + CENTER_SPREAD - 1, strip.Color(255, 0, 10, 0));
  }

  /* Fire stuff */
  uint8_t midLeft = floor((freqL1 + freqL3) / 2);
  uint8_t highLeft = floor((freqL4 + freqL5) / 2);
  uint8_t midLeftRedIntensity = fscale(0, 255, 0, 100 + (redBassIncL * 100), midLeft, 3);
  uint8_t midLeftBlueIntensity = fscale(0, 255, 0, 155 + (redBassIncL * 100), highLeft, -6);
  midLeft = fscale(0, 255, 0, CENTER_SPREAD, midLeft, 1);

  uint8_t midRight = floor((freqR1 + freqR3) / 2);
  uint8_t highRight = floor((freqR4 + freqR5) / 2);
  uint8_t midRightRedIntensity = fscale(0, 255, 0, 100 + (redBassIncR * 100), midRight, 3);
  uint8_t midRightBlueIntensity = fscale(0, 255, 0, 155 + (redBassIncR * 100), highRight, -6);
  midRight = fscale(0, 255, 0, CENTER_SPREAD, midRight, 1);

  Flicker(center_from - midLeft, middle, 255, 180 - midLeftRedIntensity, midLeftBlueIntensity, 10);
  Flicker(middle, center_to + midRight, 255, 180 - midRightRedIntensity, midRightBlueIntensity, 10);

  /* High stuff */

  uint8_t toneLeft = floor((freqL5 + freqL6) / 2);
  toneLeft = fscale(0, 255, 0, 8, toneLeft, -6);

  if(toneLeft > 3) AddLRSnake(0, 8, strip.Color(255, 255, 255, 255));

  uint8_t toneRight = floor((freqR5 + freqR6) / 2);
  toneRight = fscale(0, 255, 0, 8, toneRight, -2);

  if(toneRight > 3) AddLRSnake(strip.numPixels() - 9, strip.numPixels() - 1, strip.Color(255, 255, 255, 255));

  Flicker(0, toneLeft, 10, 255, 20, 50);
  Flicker(strip.numPixels() - toneRight, strip.numPixels() - 1, 10, 255, 20, 50);
  
  MoveSnakes();

  show();
}

void show() {
  for(uint8_t i = 0; i < strip.numPixels(); i++) {
    if (colors_center_snake[i] != strip.Color(0, 0, 0, 0)) {
      colors_center[i] = colors_center_snake[i];
    }
    
    strip.setPixelColor(i, colors_center[i]);
  }
  strip.show();
}

void AddSnake(uint8_t from, uint8_t to, uint32_t color) {
  for(int i = from; i < to; i++) {
    colors_center_snake[i] = color;
  }
}

void AddLRSnake(uint8_t from, uint8_t to, uint32_t color) {
  for(int i = from; i < to; i++) {
    colors_center[i] = color;
  }
}

void MoveSnakes() {
  for(int i = 0; i < center_from; i++) {
    colors_center_snake[i] = colors_center_snake[i + 1];
  }

  for(int i = strip.numPixels() - 1; i > center_to; i--) {
    colors_center_snake[i] = colors_center_snake[i - 1];
  }

//  for(int i = center_from - CENTER_SPREAD; i > 0; i--) {
//    colors_center[i] = colors_center[i - 1];
//  }
}

void Flicker(uint8_t from, uint8_t to, int start_r, int start_g, int start_b, int start_w) {
  for (uint16_t i = from; i < to; i++) {
    int flicker = random(0, 150);

    int r1 = start_r - floor(flicker / 1.1);
    int g1 = start_g - flicker;
    int b1 = start_b - flicker;
    int w1 = start_w - flicker;

    if (r1 < 0) r1 = 0;
    if (g1 < 0) g1 = 0;
    if (b1 < 0) b1 = 0;
    if (w1 < 0) w1 = 0;

    colors_center[i] = strip.Color(r1, g1, b1, w1);
  }
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
  for(int i = from; i < to; i++) {
    colors_center[i] = c;
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
