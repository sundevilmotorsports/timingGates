#include <Arduino.h>

#define HWSERIAL Serial1
#define CENTIMETERS 1000
#define TIMEOUT 3000

void setup() {
  Serial.begin(9600);
  HWSERIAL.begin(115200);
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
  
  Serial.println(String(distance) + "cm\tstrength: " + String(strength) + "\tstart: " + String(start));
}