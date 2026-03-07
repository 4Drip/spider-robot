#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_PWMServoDriver.h>
#include <NewPing.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Servo driver
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

// Ultrasonic sensors
#define TRIG_LEFT 5
#define ECHO_LEFT 2

#define TRIG_RIGHT 6
#define ECHO_RIGHT 3

#define TRIG_CENTER 7
#define ECHO_CENTER 4

#define MAX_DISTANCE 200

NewPing sonarLeft(TRIG_LEFT, ECHO_LEFT, MAX_DISTANCE);
NewPing sonarRight(TRIG_RIGHT, ECHO_RIGHT, MAX_DISTANCE);
NewPing sonarCenter(TRIG_CENTER, ECHO_CENTER, MAX_DISTANCE);

// Servo settings
#define SERVOMIN 150
#define SERVOMAX 600

int servoPos = SERVOMIN;
bool forward = true;

void setup() {

  Serial.begin(9600);

  // OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED non trovato");
    while(true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0,0);
  display.println("Spider Robot Test");
  display.display();
  delay(2000);

  // Servo driver
  pwm.begin();
  pwm.setPWMFreq(50);

}

void loop() {

  // Read distances
  int distLeft = sonarLeft.ping_cm();
  int distRight = sonarRight.ping_cm();
  int distCenter = sonarCenter.ping_cm();

  // OLED display
  display.clearDisplay();

  display.setCursor(0,0);
  display.println("Ultrasonic Test");

  display.setCursor(0,20);
  display.print("Left: ");
  display.print(distLeft);
  display.println(" cm");

  display.setCursor(0,35);
  display.print("Center: ");
  display.print(distCenter);
  display.println(" cm");

  display.setCursor(0,50);
  display.print("Right: ");
  display.print(distRight);
  display.println(" cm");

  display.display();

  // Move servos
  for(int i = 0; i < 8; i++) {
    pwm.setPWM(i, 0, servoPos);
  }

  if(forward) {
    servoPos += 10;
    if(servoPos >= SERVOMAX) forward = false;
  } else {
    servoPos -= 10;
    if(servoPos <= SERVOMIN) forward = true;
  }

  delay(80);
}