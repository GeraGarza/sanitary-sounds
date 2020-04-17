// Include Arduino Wire library for I2C
#include <Wire.h>
#include "SD.h"
#include "TMRpcm.h"
#include "SPI.h"

#define SPEAKER_PIN 9
#define SD_ChipSelectPin 4 // Defines the SD pin

// Define Slave I2C Address
#define SLAVE_ADDR 9

// Define Slave answer size
#define ANSWERSIZE 5

// Define string with response to Master
String answer = "Hello";
TMRpcm tmrpcm;
int timer_duration = 0;

void setup() {

  // Initialize I2C communications as Slave
  Wire.begin(SLAVE_ADDR);

  // Function to run when data received from master
  Wire.onReceive((void (*)(int))receiveEvent);

  // Setup Serial Monitor
  Serial.begin(9600);

  Serial.println("I2C Slave:");
  
  tmrpcm.speakerPin = SPEAKER_PIN;

  if (!SD.begin(SD_ChipSelectPin)) {
    Serial.println("SD fail");
    return;
  }
}

void loop() {
  delay(50);
}




void receiveEvent() {

  // Read while data received
//  from Master's: Wire.write(0);
  String response = "";
  while (0 < Wire.available()) {
    byte b = Wire.read();
    response += b;
  }
  timer_duration = atoi(response.c_str());
  Serial.println(timer_duration);
  Serial.println("Recieved event");
  tmrpcm.setVolume(5);
  
  int song_id = (rand() % 3)+1 ;  
  char q[] = "1.wav";
  q[0] = song_id + '0';
  Serial.println(q);
  tmrpcm.play(q);

  for(int i = 0; i < timer_duration; i++){
    delay(1000);
    Serial.println(i+1);
  }
  
  tmrpcm.disable();

}
