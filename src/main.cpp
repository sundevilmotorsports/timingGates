#include <Arduino.h>
#include <TimeLib.h>
#include <RH_RF95.h>
#include <Wire.h>
#include <DS3231.h>

#define CS0 10
#define G00 23

#define SCK 13
#define LED 9

#define MODEM_CONFIG RH_RF95::ModemConfigChoice::Bw500Cr45Sf128

#define HWSERIAL Serial1
#define CENTIMETERS 500
#define UPDATE_TIMEOUT 3000 //miliseconds
#define SEND_TIMEOUT 500 // milliseconds
#define LED_TIMEOUT 100 // milliseconds

#define TESTING_LIDAR false
#define TESTING_RTC false

DS3231 myRTC;
RH_RF95 driver(CS0, G00);

int starting_millis = 0;
u_int8_t starting_year = 0;
u_int8_t starting_month = 0;
u_int8_t starting_day = 0;
u_int8_t starting_hour = 0;
u_int8_t starting_minute = 0;
u_int8_t starting_second = 0;

bool radio = false;
bool century;
bool h12Flag;
bool pmFlag;

void setup() {
  pinMode(SCK, OUTPUT);
  pinMode(LED, OUTPUT);
  Serial.begin(9600);
  HWSERIAL.begin(115200);
  
  Wire.begin();

  Serial.println("\n" __FILE__ " " __DATE__ " " __TIME__);
  if(SerialUSB){
    digitalWriteFast(13, HIGH);
    time_t seconds=time(NULL);
    struct tm* current_time=localtime(&seconds);
    myRTC.setClockMode(false);  // set to 24h
    myRTC.setYear(current_time->tm_year % 100);
    myRTC.setMonth(current_time->tm_mon + 1);
    myRTC.setDate(current_time->tm_mday);
    myRTC.setHour(current_time->tm_hour);
    myRTC.setMinute(current_time->tm_min);
    myRTC.setSecond(current_time->tm_sec);
    Serial.println("Updated RTC");
  }

  int start_sec = myRTC.getSecond();
  while(start_sec == myRTC.getSecond());

  starting_millis = millis();
  starting_year = myRTC.getYear();
  starting_month = myRTC.getMonth(century);
  starting_day = myRTC.getDate();
  starting_hour = myRTC.getHour(h12Flag, pmFlag);
  starting_minute = myRTC.getMinute();
  starting_second = myRTC.getSecond();

  radio = driver.init();
  if (!radio)
    Serial.println("init failed"); 
  else {
    Serial.println("init succeded");
    driver.setFrequency(915.0); // Median of Hz range
    driver.setTxPower(RH_RF95_MAX_POWER, false); //Max power, should increase range, but try to find min because a little rude to be blasting to everyone
    // driver.setModemConfig(MODEM_CONFIG);
    driver.setSpreadingFactor(7);
    driver.setSignalBandwidth(500000);
    driver.setCodingRate4(6);
  }
}


void getTFminiData(int* distance, int* strength) {
  static char i = 0;
  char j = 0;
  int checksum = 0; 
  static int rx[9];
  if(HWSERIAL.available()) {  
    rx[i] = HWSERIAL.read();
    if(rx[0] != 0x59) {
      i = 0;
    } else if(i == 1 && rx[1] != 0x59) {
      i = 0;
    } else if(i == 8) {
      for(j = 0; j < 8; j++) {
        checksum += rx[j];
      }
      if(rx[8] == (checksum % 256)) {
        *distance = rx[2] + rx[3] * 256;
        *strength = rx[4] + rx[5] * 256;
      }
      i = 0;
    } else {
      i++;
    } 
  }  
}

int distance = 0;
int strength = 0;
int start = 0;
int prevSend = 0;
int prevLED = 0;
bool waiting = false;

uint8_t general[16];

void loop() {
  bool sent;
  getTFminiData(&distance, &strength);
  if(distance != 65535){
    if(distance < CENTIMETERS && millis() - prevLED > LED_TIMEOUT){
      digitalWriteFast(LED, LOW);
    } else {
      digitalWriteFast(LED, HIGH);
    }
    if(!waiting && distance < CENTIMETERS){
      start = millis();
      waiting = true;
      sent = false;

      general[0] = (uint8_t) 1; // Timing Gate ID
      general[1] = (uint8_t) 0; // gate number (increment with each new gate)

      general[2] = starting_second;
      general[3] = starting_minute;
      general[4] = starting_hour;
      general[5] = starting_day;
      general[6] = starting_month;
      general[7] = starting_year;

      general[8] = (starting_millis >> 24) & 0xff;
      general[9] = (starting_millis >> 16) & 0xff;
      general[10] = (starting_millis >> 8) & 0xff;
      general[11] = starting_millis & 0xff; 

      general[12] = (start >> 24) & 0xff;
      general[13] = (start >> 16) & 0xff;
      general[14] = (start >> 8) & 0xff;
      general[15] = start & 0xff; 

    } else if (waiting && millis() - start > UPDATE_TIMEOUT){
      waiting = false;
      Serial.println("ended timeout");
    }
    if(radio && millis() - prevSend > SEND_TIMEOUT){
      sent = driver.send(general, sizeof(general));
      prevSend = millis();
      Serial.println("Sent radio 1: " + String(sent) + + "\tStarting Time: " + String(starting_month) + "/" + String(starting_day) + "/" + String(starting_year) + " " + String(starting_hour) + ":" + String(starting_minute) + ":" + String(starting_second) + " millis starting: " + String(starting_millis) + " millis: " + String(millis()) + "\tstarting delta: " + String(millis()-starting_millis) + "\tdistance: " +  String(distance) + "cm");  
    }
  }
  if(Serial && TESTING_LIDAR){
    Serial.println(String(distance) + "cm\tstrength: " + String(strength) + "\tstart: " + String(start));
  }
  if(Serial && TESTING_RTC){
    Serial.println("Starting Time Test:\t" + String(starting_month) + "/" + String(starting_day) + "/" + String(starting_year) + " " + String(starting_hour) + ":" + String(starting_minute) + ":" + String(starting_second));
  }
}