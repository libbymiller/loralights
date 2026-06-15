/*

a version of https://www.yoyomachines.io/lighttouch using Lora to convey the messages
https://make.yoyomachines.io/Guide/Hardware+Build+Guide+(Mac+Software)/17?lang=en

Uses xaio esp32s3 https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/
with a lora hat - https://thepihut.com/products/xiao-esp32s3-wio-sx1262-kit-for-meshtastic-lora

Simple 2 pin button - using xiao pin D1 and ground
Used https://shop.pimoroni.com/collections/adafruit/products/neopixel-ring-16-x-5050-rgbw-leds-w-integrated-drivers which I had lying round
Also works with e.g. https://shop.pimoroni.com/products/led-rgb-clear-common-cathode?variant=44298388170 with a bit of fiddling

This antenna https://thepihut.com/products/lora-antenna-with-pigtail-868mhz-black

Advice on paramters:
    radio.begin(868.0, 500.0, 12, 8, 0x12, 22, 20); //overkill probably, slower
    radio.begin(868.0, 500.0, 11, 7, 0x12, 22, 8);//probably just as fast, longer range

*/

//lora stuff
//radiolib tested on 7.2.0
#include <RadioLib.h>

//xaio esp32 s3 https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/
SX1262 radio = new Module(41, 39, 42, 40);

// flag to indicate that a packet was received
volatile bool receivedFlag = false;

String myName;

#define BUTTONPIN D1  // the number of the pushbutton pin

#define PIN 44 //D7 (DIN)
#define LED_COUNT 12
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

Adafruit_NeoPixel strip(LED_COUNT, PIN, NEO_GRBW + NEO_KHZ800);

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  // we got a packet, set the flag
  receivedFlag = true;
}

int buttonState = 0;  // variable for reading the pushbutton status

void setAllPixels(int r, int g, int b){
    for(int i=0; i<LED_COUNT; i++) {
      strip.setPixelColor(i, strip.Color(r,g,b));
      strip.show();
    }
}

void setAllPixels2(int myCol){
    for(int i=0; i<LED_COUNT; i++) {
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(myCol)));
      strip.show();
    }

}

void setup() {
  Serial.begin(9600);
  int n = random(9999);
  myName  = String(n);

  // initialize the pushbutton pin as an input:
  pinMode(BUTTONPIN, INPUT_PULLUP);

  delay(2000);
  
  // initialize LoRa SX1262 with default settings
  Serial.print(F("[SX1262] Initializing ... "));
//int state = radio.begin();
//see https://github.com/jgromes/RadioLib/wiki/Default-configuration#sx126x---lora-modem
  int state = radio.begin(868.0, 500.0, 12, 8, 0x12, 22, 20); //That might take longer to transmit and is probably overkill
//radio.begin(868.0, 500.0, 11, 7, 0x12, 22, 8) ; //just as fast and longer range
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // set the function that will be called
  // when new packet is received
  radio.setPacketReceivedAction(setFlag);

  // start listening for LoRa packets
  Serial.print(F("[SX1262] Starting to listen ... "));
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  //flash red, green, blue as a startup signal
  delay(1000); 
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(20);

  setAllPixels(255,   0,   0);//red
  delay(1000);
  setAllPixels(0,   255,   0);//green
  delay(1000);
  setAllPixels(0,   0,   255);//blue
  delay(1000);
  setAllPixels(0,   0,   0);//white
  delay(1000);

}

int theState = 0;
int rgb = 0;

void colourLoop(){
   if(theState==1){
      Serial.println("colour changing");
      rgb += (256*2);
      setAllPixels2(rgb);
      delay(50);
   }
   if(rgb> 65536){
    rgb = 0;
   }
}

int counter = 0;
int send = 0;
int random_delay = 0;

void loraloop(){
  if(receivedFlag) {
    // reset flag
    receivedFlag = false;

    // you can read received data as an Arduino String
    String str;
    int state = radio.readData(str);

    if (state == RADIOLIB_ERR_NONE) {
      // packet was successfully received
      Serial.println(F("[SX1262] Received packet!"));

      // print data of the packet
      Serial.print(F("[SX1262] Data:\t\t"));
      Serial.println(str);
      //ignore those sent by me
      if(str.startsWith(myName) || str==""){
         Serial.println("ignoring");
      }else{
         Serial.println("got a packet to act on, changing colour");
         String col = str.substring(5);
         int myCol = col.toInt();
         setAllPixels2(myCol);

      }
    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      // packet was received, but is malformed
      Serial.println(F("CRC error!"));

    } else {
      // some other error occurred
      Serial.print(F("failed, code "));
      Serial.println(state);
    }
  }

  if(send==1){    
    Serial.print("sending packet 5 times: ");
    String myString = String(rgb);
    String toSend = myName + " "+ myString;
    Serial.println(toSend);
    for (int i = 0; i <= 5; i++) {
      random_delay = random(1000);
      delay(random_delay);//blocking but that's ok
      Serial.print("sending with delay ");
      Serial.println(random_delay);
      radio.transmit(toSend);
    }
    send = 2;//hmmm try not to carry on sending!
    radio.startReceive();
  }

}

void loop() {
  // read the state of the pushbutton value:
  buttonState = digitalRead(BUTTONPIN);

  // check if the pushbutton is pressed
  if (buttonState == LOW) {
    // turn LED on:
    Serial.println("LOW");
    theState = 1;
    //Serial.println("thestate is 1");
    counter = 0;
    send = 0;

  } else {

    theState = 0;
    counter++;
    if(counter > 10 && send == 0 && rgb>0){//three states
      send = 1;
    }
  }

  colourLoop();
  loraloop();
  delay(10);//200 breaks lora receieve stuff. I think?

}
