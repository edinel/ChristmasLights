
#include <FastLED.h>
#include <TimeLib.h>
#include <WiFi.h>
#include <stdio.h>
#include <time.h>
#include "arduino-secrets.h"
#include <Adafruit_NeoPixel.h>
#include <PsychicHttp.h>
#include <TemplatePrinter.h>

#include "String_Constants.h"



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
#define DELAYVAL 0



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
String ledColor = "off";
int buttonState = LOW;       // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
const char* PARAM_INPUT_1 = "color";

Adafruit_NeoPixel pixels(1, PIN_NEOPIXEL, NEO_GRBW + NEO_KHZ800);

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

// Create AsyncWebServer object on port 80
PsychicHttpServer server;



const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Eddie's Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
  </style>
</head>
<body>
  <h2>Eddie's Web Server</h2>
  %BUTTON_TWO%

  %BOUNCER%
  <p id="output"></p>

  <script>
    const radioButtons = document.querySelectorAll('input[name="selector"]');
    for(const radioButton of radioButtons){
      radioButton.addEventListener('change', showSelected);
    }        
    function showSelected(e) {
      var callback = new XMLHttpRequest();
      if (this.checked) {
        document.querySelector('#output').innerText = `You selected ${this.value}`;
        callback.open("GET", "/update?color="+this.value, true); 
        callback.send();
      }
      console.log(e);
    }
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
  sleep (3);
  pinMode(buttonPin, INPUT);
  // SET UP WEB SERVER 
  server.listen(80);

  // Route for root / web page
  

  server.on ("/", HTTP_GET, [](PsychicRequest *request) 
  {
    PsychicStreamResponse response(request, "text/html");
    response.beginSend();
    TemplatePrinter printer(response, RadioProcessor);  
    printer.print (index_html);
    printer.flush();
    return response.endSend();
  });

  // Send a GET request to <ESP_IP>/update?state=<inputMessage>
server.on ("/update", HTTP_GET, [](PsychicRequest *request) 
{
    PsychicStreamResponse response(request, "text/html");
    response.beginSend();
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/update?state=<inputMessage>
    if (request->hasParam("color")) 
    {
      inputMessage = request->getParam("color")->value();
      Serial.println ("I hear that the color is " + inputMessage);
      ledColor = inputMessage; //Switch the LED state
      Serial.println ("set color to" + ledColor);
    }else{
      inputMessage = "No message sent";
      inputParam = "none";
      Serial.println ("Got the wrong answer");
    }
    request->reply(200, "text/plain", "OK");
    return response.endSend();
});

/*
  // Send a GET request to <ESP_IP>/state
  server.on("/state", HTTP_GET, [] (PsychicRequest *request) {
    request->reply(200, "text/plain", String(ledState).c_str());
  });
  // Start server

*/
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
 
  uint32_t thePixelColor = pixels.Color(0, 0, 0);
  if (ledColor == "red"){
    thePixelColor = pixels.Color (50,0,0);
  }else if (ledColor == "blue"){
    thePixelColor = pixels.Color (0,0,50);
  }else if (ledColor == "green"){
    thePixelColor = pixels.Color (0,50,0);
  }else{ 
    thePixelColor = pixels.Color (0,0,0);
  }

  pixels.clear();
  for(int i=0; i<NUMPIXELS; i++) {
    pixels.setPixelColor(i, thePixelColor);
    delay(DELAYVAL);
  }
  pixels.show();

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
bool RadioProcessor (Print& output, const char *param){
  if (strcmp(param, "CSS")== 0){
    output.print (CSS_CODE);
    Serial.println ("CSS");
  }else if (strcmp (param, "BUTTONPLACEHOLDER") == 0){
    output.print ("");
    output.print ("<div class=\"radio-toolbar\">\n");
    output.print ("<input type=\"radio\" id=\"off\" name=\"selector\" value=\"off\" ");
    output.print (isThisOn("off"));
    output.print (">\n");

    output.print ("<label for=\"blue\">Off</label>\n");
    output.print ("<input type=\"radio\" id=\"blue\" name=\"selector\" value=\"blue\" ");
    output.print (isThisOn("blue"));
    output.print (">\n");
    output.print ("<label for=\"blue\">Blue</label>\n");

    output.print ("<input type=\"radio\" id=\"green\" name=\"selector\" value=\"green\" ");
    output.print (isThisOn("green"));
    output.print (">\n");
    output.print ("<label for=\"green\">Green</label>\n");

    output.print ("<input type=\"radio\" id=\"red\" name=\"selector\" value=\"red\" ");
    output.print (isThisOn("red"));
    output.print (">\n");
    output.print ("<label for=\"red\">Red</label>\n");
    output.print ("<p>");
    return true;
  }else if (strcmp (param, "BUTTON_TWO")==0 ){
    output.print("<div class='content'> \n <h3>Ripple animation on  input type radio and Checkbox</h3>");
    output.print("<div class=\"dpx\">\n");
    output.print("<div class=\'py\'>");
    output.print("<label>");
    output.print("<input type=\"radio\" class=\"option-input radio\" value=\"off\" name=\"Off\" ");
    output.print (isThisOn("off"));
    output.print("/>");
    output.print("Off\n");
    output.print ("</label>\n");
    output.print("<label>");
    output.print("<input type=\"radio\" class=\"option-input radio\" value=\"red\" name=\"Red\" ");
    output.print (isThisOn("red"));
    output.print("/>");
    output.print("Red\n");
    output.print ("</label>\n");
    output.print("<label>");
    output.print("<input type=\"radio\" class=\"option-input radio\" value=\"Green\" name=\"Green\" ");
    output.print (isThisOn("green"));
    output.print("/>");
    output.print("Green\n");
    output.print ("</label>\n");
    output.print("<label>");
    output.print("<input type=\"radio\" class=\"option-input radio\" value=\"blue\" name=\"Blue\" ");
    output.print (isThisOn("blue"));
    output.print("/>");
    output.print("Blue\n");
    output.print ("</label>\n");
    output.print ("</div>\n</div>\n</div>\n");
    return true;
    }else if (strcmp (param, "BOUNCER")==0 ){
      

      
    }else{
    return false; 
  }
}



String outputState(int number){
  if (ledState == 1){
    Serial.println ("checked");
    return "checked ";
  }
  else {

    Serial.println ("UNchecked");
    return "";
  }
  return "";
}

String isThisOn (String color){
  if (color == ledColor){
    return "checked";
  }else{
    return "";
  }
}



/*<div class="radio-toolbar">
  <input type="radio" id="radio1" name="radios" value="all" checked>
  <label for="radio1">All</label>

  <input type="radio" id="radio2" name="radios" value="false">
  <label for="radio2">Open</label>

  <input type="radio" id="radio3" name="radios" value="true">
  <label for="radio3">Archived</label>
</div>
*/


























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
