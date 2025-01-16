#include <Arduino.h>
#include <TimeLib.h>
#include <RH_RF95.h>

#define CS0 10
#define G00 23
#define SCK 13

#define MODEM_CONFIG RH_RF95::ModemConfigChoice::Bw125Cr45Sf128

#define HWSERIAL Serial1
#define CENTIMETERS 1000
#define TIMEOUT 3000

time_t RTCTime;
RH_RF95 driver(CS0, G00);

int starting_micro = 0;
int starting_time = 0;
bool radio = false;

time_t getTeensy3Time() {
  return Teensy3Clock.get();
}

void setup() {
  pinMode(SCK, OUTPUT);

  Serial.begin(9600);
  HWSERIAL.begin(115200);
  
  setSyncProvider(getTeensy3Time);
  Serial.println("\n" __FILE__ " " __DATE__ " " __TIME__);
  int start_sec = second();
  while(start_sec == second());
  starting_micro = micros();
  starting_time = now();
  
  radio = driver.init();
  if (!radio)
    Serial.println("init 1 failed"); 
  else {
    Serial.println("init 1 succeded");
    driver.setFrequency(915.0); // Median of Hz range
    driver.setTxPower(RH_RF95_MAX_POWER, false); //Max power, should increase range, but try to find min because a little rude to be blasting to everyone
    driver.setModemConfig(MODEM_CONFIG); //Bandwidth of 125, Cognitive Radio 4/5, Spreading Factor 2048
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
bool waiting = false;

uint8_t general[13];

void loop() {
  bool sent;
  getTFminiData(&distance, &strength);
  if(!waiting && distance < CENTIMETERS){
    start = millis();
    waiting = true;
    Serial.println(String(distance) + "cm\tstrength: " + String(strength) + "\tstart: " + String(start));
    bool sent = false;
    general[0] = (uint8_t) 4;
    general[1] = (uint8_t) 0; //0
    general[2] = (start >> 24) & 0xff;
    general[3] = (start >> 16) & 0xff;
    general[4] = (start >> 8) & 0xff;
    general[5] = start & 0xff; 
    general[6] = (distance >> 24) & 0xff;
    general[7] = (distance >> 16) & 0xff;
    general[8] = (distance >> 8) & 0xff;
    general[9] = distance & 0xff; 
    general[10] = (strength >> 24) & 0xff;
    general[11] = (strength >> 16) & 0xff;
    general[12] = (strength >> 8) & 0xff;
    general[13] = strength & 0xff; 

    if(radio){
      sent = driver.send(general, sizeof(general));
    }
  } else if (waiting && millis() - start > TIMEOUT){
    waiting = false;
    Serial.println("ended timeout");
    // Serial.println("Current: " + String(month()) +"/" + String(day()) + "/" + String(year()) + "\t" + String(hour()) + ":" + String(minute()) + ":" + String(second()) + "\tSeconds Since 1970: " + String(now()) + "\tMicros: " + String(micros()) + "\tStarting Seconds: " + String(starting_time) + "\tStarting micro: " + String(starting_micro));
  }
  Serial.println("Sent radio 1: " + String(sent) + "\t micros: " + String(micros()) + "\tstarting delta: " + String(micros()-starting_micro) + "\tdistance: " +  String(distance) + "cm");  
}