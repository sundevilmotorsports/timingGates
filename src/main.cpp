#include <Arduino.h>
#include <TimeLib.h>

time_t RTCTime;

#define HWSERIAL Serial1
#define CENTIMETERS 1000
#define TIMEOUT 3000

int starting_micro = 0;
int starting_time = 0;

time_t getTeensy3Time() {
  return Teensy3Clock.get();
}

void setup() {
  Serial.begin(9600);
  HWSERIAL.begin(115200);
  setSyncProvider(getTeensy3Time);
  Serial.println("\n" __FILE__ " " __DATE__ " " __TIME__);
  int start_sec = second();
  while(start_sec == second());
  starting_micro = micros();
  starting_time = now();
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

void loop() {
  getTFminiData(&distance, &strength);
  if(!waiting && distance < CENTIMETERS){
    start = millis();
    waiting = true;
  } else if (TIMEOUT && millis() - start > TIMEOUT){
    waiting = false;
  }
  Serial.println("Current: " + String(month()) +"/" + String(day()) + "/" + String(year()) + "\t" + String(hour()) + ":" + String(minute()) + ":" + String(second()) + "\tSeconds Since 1970: " + String(now()) + "\tMicros: " + String(micros()) + "\tStarting Seconds: " + String(starting_time) + "\tStarting micro: " + String(starting_micro));
  Serial.println(String(distance) + "cm\tstrength: " + String(strength) + "\tstart: " + String(start));
}