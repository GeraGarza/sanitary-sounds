
#include <NewPing.h> // Provides Accurate, Non-blocking sensor data from the Ultrasonic Sensor
#include <Ewma.h>    // Smoothing library - used to remove jitter from sensor data
#include <cppQueue.h> // Queue Library - used to implment mean subttraction so sensor can work in more sinks. 
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

int timer_duration = 20; 
int queue_length = 10;
int queue_change_length = 4;
int minWaitTime = 25;
int waitingTime = 0;
boolean isWaiting = true;
int difference_threshold = 3; //the amount of deviation from the mean that will trigger the countdown.
float min_mean = 0.0;
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);
Servo countServo;
Queue window(sizeof(float), queue_length, IMPLEMENTATION, OVERWRITE);
Queue change(sizeof(float), queue_change_length, IMPLEMENTATION, OVERWRITE);


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


  //get filtered data from the sensor
  float filtered1 = adcFilter1.filter(sonar.ping_cm());
  //  Serial.println(sonar.ping_cm());

  //subtract the mean so sensor data is centered around zero.
  float mean_value = filtered1 - meanZero(filtered1);
  //This allows the sensor to adjust to different environments and look for change rather than a preset threshold.

  
  float mean_of_mean = 0;
  for (int i = 1; i < queue_change_length; i++) {
      float cur_mean;
      change.peekIdx(&cur_mean, i);
      mean_of_mean = mean_of_mean + (int)cur_mean;
  }
  
  //calculate the mean and return.
  int mean_means = mean_of_mean / queue_change_length;
  Serial.println((String)mean_value + " " + (String)mean_means);
  if (isWaiting) {
    Serial.println("Waiting Time: " + (String)waitingTime + " out of " + (String)minWaitTime);
    waitingTime++;
  }
  //If the sensor reads something significantly different from what it's normally seeing (the empty sink)
  //we start the countdown!
  if ( abs(mean_means) >= difference_threshold) {
    //    has waited enought frames
    if (!isWaiting) { 
      Serial.println("Write data to slave");
      Wire.beginTransmission(SLAVE_ADDR);
      Wire.write(timer_duration);
      Wire.endTransmission();
      Serial.println("Receive data");

      countdownServo();
    }

  }

  // if 
  if (waitingTime >= minWaitTime) {
    isWaiting = false;
  }

  delay(500);
  change.push(&mean_value);

}

//this moves the servo 4.5 degrees every half second
//to smooth out the movemnt (vs 9 degrees every second)
//for 20 seconds.
void countdownServo() {
  Serial.println("Counting down");
  int halfseconds = timer_duration * 2;

  for (int i = halfseconds; i >= 0; i--) {

    countServo.write((int)(i * 4.5));
    delay(500);
  }

  //reset the servo, clear the queue
  countServo.write(180);
  clearQueue();
  delay(1000);
  waitingTime = 0;
  isWaiting = true;
}

void clearQueue() {
  for (int i = 0; i < queue_length; i++) {
    float queue_zeroer = 0;
    window.push(&queue_zeroer);
  }
}


//helper function to calculate a moving mean.
int meanZero(float smoothedVal) {

  window.push(&smoothedVal); //push the most recent reading into the queue

  int retval = 0;

  //iterate through the queue and add values to return val
  for (int i = 0; i < queue_length; i++) {
    float for_mean;
    window.peekIdx(&for_mean, i);
    retval = retval + (int)for_mean;
  }

  //calculate the mean and return.
  int mean = retval / queue_length;
  return mean;
}
