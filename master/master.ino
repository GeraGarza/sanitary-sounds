
//Packets to install
#include <NewPing.h> // Provides Accurate, Non-blocking sensor data from the Ultrasonic Sensor
#include <Ewma.h>    // Smoothing library - used to remove jitter from sensor data
#include <cppQueue.h> // From Github: Queue Library - used to implment mean subttraction so sensor can work in more sinks. 

#include <Servo.h>
#include <Wire.h>


// Pins
#define SERVO_PIN 7
#define TRIGGER_PIN  8  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     10  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 200 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

#define IMPLEMENTATION FIFO //Queue implementation as First in First out
#define OVERWRITE      true //Queue will overwrite oldest value when full
#define SLAVE_ADDR 9
// Define Slave answer size
#define ANSWERSIZE 5

int timer_duration = 20                                                                   ; 
int queue_length = 10;
int minWaitTime = 10;
int waitingTime = 0;
boolean isWaiting = true;
int difference_threshold = 8; //the amount of deviation from the mean that will trigger the countdown.
float min_mean = 0.0;
int prev_total_close = 0;
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);
Servo countServo;
Queue window(sizeof(float), queue_length, IMPLEMENTATION, OVERWRITE);


Ewma adcFilter1(0.05); //filter used to smooth sensor data
void setup() {

  // Initialize I2C communications as Master
  Wire.begin();

  // Setup serial monitor
  Serial.begin(9600);
  Serial.println("I2C Master:");
  countServo.attach(SERVO_PIN);
  countServo.write(180);
  delay(700);
  countServo.write(0);
  delay(700);
  countServo.write(180);
  
  clearQueue();
  Serial.print("Waiting Time Needed: ");
  Serial.println(minWaitTime);

}

void loop() {

  // Wait 50ms between ultrasonic pings (about 20 pings/sec).
  delay(50);
  int distance = sonar.ping_cm();

  //get filtered data from the sensor
  float filtered1 =(distance);

  //This allows the sensor to adjust to different environments and look for change rather than a preset threshold.

  int total_close = 0;
  for (int i = 0; i < queue_length; i++) {
    float value;
    window.peekIdx(&value, i);
    if( (int)value > 1 && (int)value < 80){
        total_close ++;
      }
  }
  


  Serial.println("Callibrated Zero: "+(String)total_close+ " (>2)");
  if(total_close > 4){
    clearQueue();
  }
  if (isWaiting) {
    Serial.println("Time: " + (String)waitingTime + " / " + (String)minWaitTime);
    Serial.println("");
    waitingTime++;
  }
  //If the sensor reads something significantly different from what it's normally seeing (the empty sink)
  //we start the countdown!
  if ( total_close ==  2 && prev_total_close == 1) {
    //    has waited enought frames
    if (!isWaiting) { 
      Serial.println("Write data to slave");
      Wire.beginTransmission(SLAVE_ADDR);
      Wire.write(timer_duration);
      Wire.endTransmission();
      Serial.println("Slave playing audio");
      
      countdownServo();
    }

  }

  if (waitingTime >= minWaitTime) {
    isWaiting = false;
  }

  delay(500);

  prev_total_close = total_close;
}

// moves the servo 2.25 degrees every quarter (smooth)
void countdownServo() {
  Serial.println("Counting down");
  int quarterseconds = timer_duration * 4;
  int seconds = 0;
  for (int i = quarterseconds; i >= 0; i--) {
    int angle = i *  (180/quarterseconds);
    countServo.write(angle);
    if (seconds%4 == 0){
      Serial.println((String)(seconds/4));
    }
    seconds ++;
    delay(250);
  }

  //reset the servo, clear the queue
  countServo.write(180);
  clearQueue();
  delay(1000);
  waitingTime = 0;
  isWaiting = true;
}

// make all index's 0
void clearQueue() {
  for (int i = 0; i < queue_length; i++) {
    float zero = 0;
    window.push(&zero);
  }
}
