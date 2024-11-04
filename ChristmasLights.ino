
#include <FastLED.h>
#include <TimeLib.h>
#include <WiFi.h>
#include <stdio.h>
#include <time.h>
#include "arduino-secrets.h"
#include <Adafruit_NeoPixel.h>
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

#define NUMPIXELS 1
#define DELAYVAL 2



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

const int output = 13;
const int buttonPin = BUTTON;
// Variables will change:
int ledState = LOW;         // the current state of the output pin
int buttonState = LOW;       // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
const char* PARAM_INPUT_1 = "state";

Adafruit_NeoPixel pixels(1, PIN_NEOPIXEL, NEO_GRBW + NEO_KHZ800);

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);



const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>ESP Web Server</h2>
  %BUTTONPLACEHOLDER%
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?state=1", true); }
  else { xhr.open("GET", "/update?state=0", true); }
  xhr.send();
}

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var inputChecked;
      var outputStateM;
      if( this.responseText == 1){ 
        inputChecked = true;
        outputStateM = "On";
      }
      else { 
        inputChecked = false;
        outputStateM = "Off";
      }
      document.getElementById("output").checked = inputChecked;
      document.getElementById("outputState").innerHTML = outputStateM;
    }
  };
  xhttp.open("GET", "/state", true);
  xhttp.send();
}, 1000 ) ;
</script>
</body>
</html>
)rawliteral";




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
  sleep (10);
  pinMode(buttonPin, INPUT);
  // SET UP WEB SERVER 

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/update?state=<inputMessage>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/update?state=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      ledState = !ledState; //Switch the LED state
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.print("here...");
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });

  // Send a GET request to <ESP_IP>/state
  server.on("/state", HTTP_GET, [] (AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(ledState).c_str());
  });
  // Start server
  server.begin();


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
  //Serial.println ("Top of loop");

  pixels.clear();
  if (ledState == 1){
    for(int i=0; i<NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(0, 25, 0));
      pixels.show();
      delay(DELAYVAL);
    }
  }else{
    pixels.clear();
    pixels.show();
  }

  int reading = !digitalRead(buttonPin); //when button pushed, reading is 1
  if (reading){
    Serial.print("Here ");
    Serial.println (reading);

  }

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      // only toggle the LED if the new button state is HIGH
      if (buttonState == HIGH) {
        ledState = !ledState;
      }
    }
  }
  // digitalWrite(output, ledState);
  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastButtonState = reading;

// BELOW THIS IS ALL THE FAST LED STUFF

  // Call the current pattern function once, updating the 'leds' array
  //gPatterns[gCurrentPatternNumber](); //this is a clever "oh no switch statements scare me" bit.  Casting a string as the name of a function?  come on.
 // Serial.print ("gCurrentPatternNumber");
 // Serial.println (gCurrentPatternNumber);
  
 // Serial.println ("After first pixels invoke");



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
//  FastLED.show();  
  // insert a delay to keep the framerate modest
//  FastLED.delay(1000/FRAMES_PER_SECOND); 

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
    Serial.println(WiFi.localIP());

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





// Replaces placeholder with button section in your web page
String processor(const String& var){
  //Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons ="";
    String outputStateValue = outputState();
    buttons+= "<h4>Output - GPIO 2 - State <span id=\"outputState\"></span></h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" " + outputStateValue + "><span class=\"slider\"></span></label>";
    return buttons;
  }
  return String();
}

String outputState(){
  if (ledState == 1){
    Serial.println ("checked");
    return "checked";
  }
  else {

    Serial.println ("UNchecked");
    return "";
  }
  return "";
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

