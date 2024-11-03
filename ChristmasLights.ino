/*EDDIE NOTE THIS IS A COPY OF THE FASTLED-TEST SKETCH AND NEEDS MASSIVE EDITING*/

#include <FastLED.h>
#include <TimeLib.h>
#include <WiFi.h>
#include <stdio.h>
#include <time.h>
#include "arduino-secrets.h"
#ifdef ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
#else
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

//FASTLED_USING_NAMESPACE

// FastLED "100-lines-of-code" demo reel, showing just a few 
// of the kinds of animation patterns you can quickly and easily 
// compose using FastLED.  
//
// This example also shows one easy way to define multiple 
// animations patterns and have them automatically rotate.
//
// -Mark Kriegsman, December 2014


#define DATA_PIN_1    27
#define DATA_PIN_2    33 
//#define CLK_PIN   4
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB
#define NUM_LEDS_1    400
#define NUM_LEDS_2    200
#define BRIGHTNESS          255  
#define FRAMES_PER_SECOND  120
#define NUM_FUNCTIONS 4

CRGB leds_1[NUM_LEDS_1];
CRGB leds_2[NUM_LEDS_2];
bool debug = true;
#define hostname "BackYard-Xmas-Arduino"

void setup() {
  
  // SET UP SERIAL
  Serial.begin(115200);
    while (!Serial) {
      ;  // wait for serial port to connect. Needed for native USB port only
    }
  Serial.println ("0 setup");
  Serial.flush();
  delay(3000); // 3 second delay for recovery
  Serial.println ("1 setup");
  Serial.flush();
  

  // SET UP LEDS tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN_1,COLOR_ORDER>(leds_1, NUM_LEDS_1).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE,DATA_PIN_2,COLOR_ORDER>(leds_2, NUM_LEDS_2).setCorrection(TypicalLEDStrip);  
  FastLED.setBrightness(BRIGHTNESS); // set master brightness control
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  
  
  // SET UP WIFI 
   Connect_to_Wifi();  // Like it says
  if (debug) { Print_Wifi_Status(); }
  sleep (20);
  
  
  
  
  
  Serial.println ("Exting setup");
  Serial.flush();


}


// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
// SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm };
// SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon };


uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
  
void loop()
{
  Serial.println ("Top of loop");
  // Call the current pattern function once, updating the 'leds' array
  //gPatterns[gCurrentPatternNumber](); //this is a clever "oh no switch statements scare me" bit.  Casting a string as the name of a function?  come on.
  Serial.println (gCurrentPatternNumber);
  
  switch(gCurrentPatternNumber){
    case 1:
      rainbow();
      break;
    case 2:
      rainbowWithGlitter();
      break;
    case 3:
      confetti();
      break;
    case 4:
      sinelon();
      break;
    }


  // send the 'leds' array out to the actual LED strip
  FastLED.show();  
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/FRAMES_PER_SECOND); 

  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  EVERY_N_SECONDS( 10 ) { nextPattern(); } // change patterns periodically
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % NUM_FUNCTIONS;
}

void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds_1, NUM_LEDS_1, gHue, 7);
  fill_rainbow( leds_2, NUM_LEDS_2, gHue, 7);
}

void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds_1[ random16(NUM_LEDS_1) ] += CRGB::White;
    leds_2[ random16(NUM_LEDS_2) ] += CRGB::White;
  }
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds_1, NUM_LEDS_1, 10);
  int pos1 = random16(NUM_LEDS_1);
  leds_1[pos1] += CHSV( gHue + random8(64), 200, 255);

  fadeToBlackBy( leds_2, NUM_LEDS_2, 10);
  int pos2 = random16(NUM_LEDS_2);
  leds_2[pos2] += CHSV( gHue + random8(64), 200, 255);
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds_1, NUM_LEDS_1, 20);
  int pos1 = beatsin16( 13, 0, NUM_LEDS_1-1 );
  leds_1[pos1] += CHSV( gHue, 255, 192);

  fadeToBlackBy( leds_2, NUM_LEDS_2, 20);
  int pos2 = beatsin16( 13, 0, NUM_LEDS_2-1 );
  leds_2[pos2] += CHSV( gHue, 255, 192);
}


void Connect_to_Wifi() {
  int status = WL_IDLE_STATUS;
  Serial.print("Attempting to connect to WiFi, ");
  Serial.print("SSID ");
  Serial.println(ssid);
  WiFi.setHostname(hostname);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}

void Print_Wifi_Status() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

/*
void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS_1; i++) { //9948
    leds_1[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
  for( int i = 0; i < NUM_LEDS_2; i++) { //9948
    leds_2[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }

}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds_1, NUM_LEDS_1, 20);
  uint8_t dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds_1[beatsin16( i+7, 0, NUM_LEDS_1-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }

fadeToBlackBy( leds_2, NUM_LEDS_2 20);
  uint8_t dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds_2[beatsin16( i+7, 0, NUM_LEDS_2-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }


}


*/


// HTML Code Ideas
/*

<a href="KFC.html" class="button">KFC</a>
 <a href="Pizza_hut.html" class="button">Pizza hut</a>
 
 
 
 a.button { 
   display: inline-block;
   padding: 5px;
   border: 1px solid #999;
   background: #aaa;
   color: #333;
   text-decoration: none;
 }


*/