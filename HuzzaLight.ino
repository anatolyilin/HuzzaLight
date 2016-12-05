#include <Time.h>
#include <TimeLib.h>
#include <Time.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <CapacitiveSensor.h>

// Setup LED strip
#include <FastLED.h>
#define NUM_LEDS 60
#define DATA_PIN 4
#define CLOCK_PIN 5
const int HOURLED = 29; // LED 30 top middle. 

// Setup touch library
#define SENSEPIN_1    12
#define SENSEPIN_2   13

// Check if required
#define TIME_MSG_LEN  11   // time sync to PC is HEADER followed by Unix time_t as ten ASCII digits
#define TIME_HEADER  'T'   // Header tag for serial time sync message
#define TIME_REQUEST  7    // ASCII bell character requests a time sync message 

// Setup WIFI 
//char ssid[] = "Fsociety";  //  your network SSID (name)
//char pass[] = "nuke80-money";       // your network password
char ssid[] = "telenet-6BE62";
char pass[] = "zAKmR0TMph4c"; 

// NTP settings
static const char ntpServerName[] = "us.pool.ntp.org";
const int timeZone = 1;     // Central European Time
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets
int previousMinute = 999;
int previousSecond = 999;
time_t getNtpTime();
void sendNTPpacket(IPAddress &address);

CapacitiveSensor cs_4_2 = CapacitiveSensor(SENSEPIN_1, SENSEPIN_2);
int Threshold_Sensor; // threshold value for touch sensor

CRGB leds[NUM_LEDS]; // current state of leds

// final state of leds


// if debugState = false the serial won't be initiliazed.
const int debugState = false;

// housekeeping variables 
int StateThreshold = millis(); 
int state = 1; 
int baseLineBrightness = 168; //brightness for Klok 
void setup() {
  setupLED();
  RunTestLED();
  if (debugState) {
      Serial.begin(9600);
      //Serial.setDebugOutput(true);
      delay(250);
      Serial.println("congratulations, DEBUG state enabled.");
  } 
  setupWifi(); 
  setupNTPtime();

//  setupTouchSensor();
  Blinking(2, 100,  CRGB::Green);
}
void loop() {
  // time housekeeping, print time every 5sec, update time every 5 min
  timeHousekeepingtime(5,5);

  if (statechanged() && ((-StateThreshold + millis()) > 500) ) {
    StateThreshold = millis();
    if(state == 1) {
      PrintDebug(" Lamp ");
      state = 0; 
      PerformStateChange();
    } else { // consider adding more states
      PrintDebug(" Klok ");
      state = 1;
      PerformStateChange();
    }
  }


 if (state == 0) {
  //lamp
 
  //FastLED.setBrightness( 0..255 );
  //fill_solid( leds, NUM_LEDS, CRGB(50,0,200));
 }



  
}

void updateKlok() {
  // every 10 min, update time
  PrintlnDebug(" ******** ");
  PrintlnDebug(" updating time... ");
  PrintDebug(" time before update: ");
  digitalClockDisplay();
  setSyncProvider(getNtpTime);
  PrintDebug(" time after update: ");
  digitalClockDisplay();
  PrintlnDebug(" ******** ");
}
void digitalClockDisplay() {
  // digital clock display of the time
  PrintDebug(hour());
  printDigits(minute());
  printDigits(second());
  PrintDebug(" ");
  PrintDebug(day());
  PrintDebug(" ");
  PrintDebug(month());
  PrintDebug(" ");
  PrintDebug(year());
  PrintlnDebug("");
}
void printDigits(int digits) {
  // utility function for digital clock display: prints preceding colon and leading 0
  PrintDebug(":");
  if (digits < 10)
    PrintDebug('0');
  PrintDebug(digits);
}
time_t getNtpTime() {
  IPAddress ntpServerIP; // NTP server's ip address
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  PrintlnDebug("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  PrintDebug(ntpServerName);
  PrintDebug(": ");
  PrintlnDebug(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      PrintlnDebug("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  PrintlnDebug("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}
void sendNTPpacket(IPAddress &address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
void setupWifi() {
  PrintDebug("Connecting to ");
  PrintDebug(ssid);
  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    PrintDebug(".");
    RunTestLED();
  }
}
void PrintDebug(String text){
  if (debugState) {
    Serial.print(text);
  }
}
void PrintlnDebug(String text){
  if (debugState) {
    Serial.println(text);
  }
}
void PrintDebug(char text){
  if (debugState) {
    Serial.print(text);
  }
}
void PrintlnDebug(char text){
  if (debugState) {
    Serial.println(text);
  }
}
void setupNTPtime() {
  PrintlnDebug("Starting UDP");
  Udp.begin(localPort);
  PrintDebug("Local port: ");
  PrintlnDebug(" " + Udp.localPort());
  PrintlnDebug("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
  setTime(getNtpTime());
}
void setupLED() {
  delay(2000); // wait a bit to get LED's going
  FastLED.addLeds<DOTSTAR, DATA_PIN, CLOCK_PIN, BGR>(leds, NUM_LEDS);
}
void setupTouchSensor(){
  // your final opportunity to get threshold value right 

  // step 1: set LED to indicate we take make measurement for X seconds, take average value
  // step 2: set LED to force user to touch the ring, take average value
  // step 3: compare values and get threshold value
  Threshold_Sensor = 1000;
}
void timeHousekeepingtime(int printInterval, int updateInterval) {
  if (timeStatus() == timeNotSet) {
    PrintlnDebug("[WARNING] time not set ");
  }
    // print time to serial every 5 seconds. [only in debug mode]
  if (second() % printInterval == 0 && second() != previousSecond && debugState ) {
    digitalClockDisplay();
    previousSecond = second();
  }

  if (minute() % updateInterval == 0 && minute() != previousMinute ) {
    updateKlok();
    previousMinute = minute();
  }
}
int ConvertHourtoLED(int hourin){
  // LEDS between hours:  
  int step = NUM_LEDS / 12; // 60 leds / 12 hours = 5 LED/hour
  
  // 12 uur = HOURLED             LED 29
  // 1 uur = HOURLED + step       LED 34
  // 2 uur = HOURLED + step*2     LED 39
  // 3 uur = HOURLED + step*3     LED 44
  // 4 uur = HOURLED + step*4     LED 49
  // 5 uur = HOURLED + step*5     LED 54
  // 6 uur = HOURLED + step*6 ->  LED 59
  // 7 uur = HOURLED + step*7     LED 4 .    64
  // 8 uur = HOURLED + step*8     LED 9 .    69
  // 9 uur = HOURLED + step*9     LED 14     74
  // 10 uur = HOURLED + step*10   LED 19 .   79
  // 11 uur  = HOURLED + step*11  LED 24 .   84

  if (hourin == 12) {
    return HOURLED;
  } else if ( hourin < 7 ){
    return HOURLED + step*hourin;
  } else {
    return HOURLED + step*hourin - 60;
  }
}
int ConvertHourtoLED(){
  return ConvertHourtoLED(hour());
}
int ConvertMinutetoLED(int minin){
  if (minin < 31) {
    return HOURLED + minin;
  } else {
    return HOURLED + minin - 30;
  }
  return 0;
}
int ConvertMinutetoLED(){
  return ConvertMinutetoLED(minute());
}
void RunTestLED(){ 
     for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) {
      // Turn our current led on to white, then show the leds
      leds[whiteLed] = CRGB::Red;

      // Show the leds (only one of which is set to white, from above)
      FastLED.show();

      // Wait a little bit
      delay(10);

      // Turn our current led back to black for the next loop around
      leds[whiteLed] = CRGB::Black;
   }
}
void Blinking(int times, int time_delay, const struct CRGB& color) {
   for (int i = 0 ; i < times ; i ++){
   fill_solid(leds, NUM_LEDS, color);
   FastLED.show();
   delay(time_delay);
   fill_solid(leds, NUM_LEDS, CRGB::Black);
   FastLED.show();
   delay(time_delay);
   }
}
boolean statechanged() {
  return false;
    long CapSensorValue =  cs_4_2.capacitiveSensor(30);
    PrintlnDebug(CapSensorValue);                  // print sensor output 1
    if (CapSensorValue > Threshold_Sensor) {
      PrintlnDebug("touched");
      return true;
    } else {
     return false;
    }
}
void PerformStateChange(){
  if(state == 1) {
      // zet klok om naar lamp
      // zet brightness van 0 > max 
      // zet klok leds brightness van baseline > max
      // getcolor of led -> adjust hue
      
    } else if (state == 0 ) {
      // zet lamp naar klok
      // zet all leds > baseline (klok leds)
      // zet alle niet klok leds > 0 
    }
}
void setKwadrant(const struct CRGB& color){
    // kwadrant 1: led 0,5, 10, 15 aan 
    // kwadrant 2: led 15,20, 25 , 30 aan 
    // kwadrant 3: led 30,35, 40, 45 aan 
    // kwadrant 4: led 45,50, 55, 60 aan

    if (hour() > 8) {
      var kwadrant = 4;
    } else if (hour() > 5) {
      var kwadrant = 3;
    } else if (hour() > 2) {
      var kwadrant = 2;
    } else {
      var kwadrant = 1;
    }
      int startled = 0;
 switch (kwadrant) {
    case 1:
      // kwadrant 1: led 0,5, 10, 15 aan 
      startled = 0;
      break;
    case 2:
      startled = 15;
      break;
    case 3:
      startled = 30;
      break;
     case 4:
      startled = 45;
      break;
    default: 
      // if nothing else matches, do the default
        PrintlnDebug(" kwadrant moet tussen 1 en 4 zijn");
    break;
  }
  for (int i = 0; i < 20 ;i = i +5 ) {
     leds[i] = color;
     FastLED.show();
  }
 }
void setKwadrant(int kwadrant, const struct CRGB& color ){
    // kwadrant 1: led 0,5, 10, 15 aan 
    // kwadrant 2: led 15,20, 25 , 30 aan 
    // kwadrant 3: led 30,35, 40, 45 aan 
    // kwadrant 4: led 45,50, 55, 60 aan
      int startled = 0;
 switch (kwadrant) {
    case 1:
      // kwadrant 1: led 0,5, 10, 15 aan 
      startled = 0;
      break;
    case 2:
      startled = 15;
      break;
    case 3:
      startled = 30;
      break;
     case 4:
      startled = 45;
      break;
    default: 
      // if nothing else matches, do the default
        PrintlnDebug(" kwadrant moet tussen 1 en 4 zijn");
    break;
  }
  for (int i = 0; i < 20 ;i = i +5 ) {
     leds[i] = color;
     FastLED.show();
  }
 }
void setKwadrant(,const struct CHSV& color){
    // kwadrant 1: led 0,5, 10, 15 aan 
    // kwadrant 2: led 15,20, 25 , 30 aan 
    // kwadrant 3: led 30,35, 40, 45 aan 
    // kwadrant 4: led 45,50, 55, 60 aan

    if (hour() > 8) {
      var kwadrant = 4;
    } else if (hour() > 5) {
      var kwadrant = 3;
    } else if (hour() > 2) {
      var kwadrant = 2;
    } else {
      var kwadrant = 1;
    }
      int startled = 0;
 switch (kwadrant) {
    case 1:
      // kwadrant 1: led 0,5, 10, 15 aan 
      startled = 0;
      break;
    case 2:
      startled = 15;
      break;
    case 3:
      startled = 30;
      break;
     case 4:
      startled = 45;
      break;
    default: 
      // if nothing else matches, do the default
        PrintlnDebug(" kwadrant moet tussen 1 en 4 zijn");
    break;
  }
  for (int i = 0; i < 20 ;i = i +5 ) {
     leds[i] = color;
     FastLED.show();
  }
 }
void setKwadrant(int kwadrant, const struct CHSV& color){
    // kwadrant 1: led 0,5, 10, 15 aan 
    // kwadrant 2: led 15,20, 25 , 30 aan 
    // kwadrant 3: led 30,35, 40, 45 aan 
    // kwadrant 4: led 45,50, 55, 60 aan
      int startled = 0;
 switch (kwadrant) {
    case 1:
      // kwadrant 1: led 0,5, 10, 15 aan 
      startled = 0;
      break;
    case 2:
      startled = 15;
      break;
    case 3:
      startled = 30;
      break;
     case 4:
      startled = 45;
      break;
    default: 
      // if nothing else matches, do the default
        PrintlnDebug(" kwadrant moet tussen 1 en 4 zijn");
    break;
  }
  for (int i = 0; i < 20 ;i = i +5 ) {
     leds[i] = color;
     FastLED.show();
  }
}
void setTrailToLED() {
   
}

