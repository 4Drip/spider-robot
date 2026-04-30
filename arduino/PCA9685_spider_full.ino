#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <NewPing.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── OLED ─────────────────────────────────────────────────────
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ── PCA9685 ──────────────────────────────────────────────────
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);
#define SERVO_FREQ  50
#define SERVOMIN   150
#define SERVOMAX   600

// I2C bus: A4=SDA, A5=SCL  (shared with OLED at 0x3C, PCA9685 at 0x40)
// Power wiring (no code needed – physical connections only):
//   Power bank USB -> Arduino 5V & GND pins (top of board)
//     5V pin -> HC-SR04 #1,#2,#3 VCC
//     5V pin -> OLED VCC
//     GND    -> all sensor GND, OLED GND
//   Power bank -> PCA9685 V+ screw terminal -> servo rail (separate supply)
//   PCA9685 GND -> Arduino GND (common ground)
uint16_t angleToPulse(float a) {
  a = constrain(a, 0, 180);
  return (uint16_t)(SERVOMIN + (a / 180.0f) * (SERVOMAX - SERVOMIN));
}
void pwm_write(int ch, float a) { pwm.setPWM(ch, 0, angleToPulse(a)); }

const int servo_channel[4][3] = {
  { 0, 1, 2}, { 3, 4, 5}, { 6, 7, 8}, { 9,10,11}
};

#define TRIG_FRONT  9
#define ECHO_FRONT  10
#define TRIG_LEFT   5
#define ECHO_LEFT   6
#define TRIG_RIGHT  A0
#define ECHO_RIGHT  A1

#define MAX_DIST    200
#define DANGER_DIST  10
#define WARN_DIST    25

NewPing sonar_front(TRIG_FRONT, ECHO_FRONT, MAX_DIST);
NewPing sonar_left (TRIG_LEFT,  ECHO_LEFT,  MAX_DIST);
NewPing sonar_right(TRIG_RIGHT, ECHO_RIGHT, MAX_DIST);

unsigned int dist_front = 99, dist_left = 99, dist_right = 99;
unsigned long last_sonar_ms = 0;
#define SONAR_INTERVAL 120

// ── Modalita ─────────────────────────────────────────────────
bool ai_mode = false;

// ── Faccine: usiamo #define invece di enum ────────────────────
// Cosi il compilatore Arduino non si confonde con il pre-processing
#define FACE_NEUTRAL  0
#define FACE_HAPPY    1
#define FACE_SAD      2
#define FACE_ANGRY    3
#define FACE_ANNOYED  4
#define FACE_CUTE     5
#define FACE_SCARED   6
#define FACE_WALK     7
#define FACE_TURN_L   8
#define FACE_TURN_R   9

int current_face     = FACE_NEUTRAL;
int last_drawn_face  = -1;

// ── Helper: ellisse piena ─────────────────────────────────────
void gfx_ellipse(int cx, int cy, int rx, int ry, uint16_t col) {
  for (int dy = -ry; dy <= ry; dy++)
    for (int dx = -rx; dx <= rx; dx++)
      if ((long)dx*dx*ry*ry + (long)dy*dy*rx*rx <= (long)rx*rx*ry*ry)
        display.drawPixel(cx+dx, cy+dy, col);
}

// ── Etichetta centrata in fondo ───────────────────────────────
void printLabel(const char* txt) {
  display.setTextSize(1);
  int16_t bx, by; uint16_t bw, bh;
  display.getTextBounds(txt, 0, 0, &bx, &by, &bw, &bh);
  display.setCursor((128 - (int)bw) / 2, 55);
  display.print(txt);
}

// ── Disegna faccina ───────────────────────────────────────────
void drawFace(int face) {
  if (face == last_drawn_face) return;
  last_drawn_face = face;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.drawRoundRect(0, 0, 128, 64, 8, SSD1306_WHITE);

  const int ex1=38, ex2=90, ey=22, mx=64, my=46;

  switch (face) {

    case FACE_HAPPY:
      display.fillCircle(ex1, ey, 8, SSD1306_WHITE);
      display.fillCircle(ex2, ey, 8, SSD1306_WHITE);
      display.fillRect(ex1-9, ey-9, 18, 10, SSD1306_BLACK);
      display.fillRect(ex2-9, ey-9, 18, 10, SSD1306_BLACK);
      for (int i=-18; i<=18; i++)
        display.drawPixel(mx+i, my+(int)(0.03f*i*i), SSD1306_WHITE);
      display.fillCircle(ex1-12, ey+14, 5, SSD1306_WHITE);
      display.fillCircle(ex2+12, ey+14, 5, SSD1306_WHITE);
      printLabel("HAPPY!");
      break;

    case FACE_SAD:
      display.fillCircle(ex1, ey, 7, SSD1306_WHITE);
      display.fillCircle(ex2, ey, 7, SSD1306_WHITE);
      display.fillCircle(ex1, ey+2, 3, SSD1306_BLACK);
      display.fillCircle(ex2, ey+2, 3, SSD1306_BLACK);
      display.fillCircle(ex1+2, ey+10, 2, SSD1306_WHITE);
      display.fillCircle(ex2+2, ey+10, 2, SSD1306_WHITE);
      for (int i=-14; i<=14; i++)
        display.drawPixel(mx+i, my-(int)(0.025f*i*i)+5, SSD1306_WHITE);
      printLabel("...");
      break;

    case FACE_ANGRY:
      display.fillCircle(ex1, ey, 7, SSD1306_WHITE);
      display.fillCircle(ex2, ey, 7, SSD1306_WHITE);
      display.fillCircle(ex1, ey, 3, SSD1306_BLACK);
      display.fillCircle(ex2, ey, 3, SSD1306_BLACK);
      display.drawLine(ex1-8, ey-10, ex1+3, ey-5, SSD1306_WHITE);
      display.drawLine(ex1-8, ey-9,  ex1+3, ey-4, SSD1306_WHITE);
      display.drawLine(ex2+8, ey-10, ex2-3, ey-5, SSD1306_WHITE);
      display.drawLine(ex2+8, ey-9,  ex2-3, ey-4, SSD1306_WHITE);
      display.drawLine(mx-14, my+2, mx+14, my+2, SSD1306_WHITE);
      display.drawLine(mx-14, my+2, mx-18, my+6, SSD1306_WHITE);
      display.drawLine(mx+14, my+2, mx+18, my+6, SSD1306_WHITE);
      printLabel("GRRRR!");
      break;

    case FACE_ANNOYED:
      display.fillCircle(ex1, ey, 7, SSD1306_WHITE);
      display.fillCircle(ex1, ey, 3, SSD1306_BLACK);
      display.fillCircle(ex2, ey, 7, SSD1306_WHITE);
      display.fillRect(ex2-8, ey-8, 16, 7, SSD1306_BLACK);
      display.fillCircle(ex2, ey+1, 3, SSD1306_BLACK);
      display.drawLine(mx-12, my, mx+12, my, SSD1306_WHITE);
      printLabel("-_-");
      break;

    case FACE_CUTE:
      display.setTextSize(2);
      display.setCursor(ex1-10, ey-8); display.print(">");
      display.setCursor(ex2-4,  ey-8); display.print("<");
      display.setTextSize(1);
      display.fillCircle(mx, my-8, 2, SSD1306_WHITE);
      display.drawLine(mx-12, my,   mx-6,  my+6, SSD1306_WHITE);
      display.drawLine(mx-6,  my+6, mx,    my,   SSD1306_WHITE);
      display.drawLine(mx,    my,   mx+6,  my+6, SSD1306_WHITE);
      display.drawLine(mx+6,  my+6, mx+12, my,   SSD1306_WHITE);
      printLabel("UwU");
      break;

    case FACE_SCARED:
      display.fillCircle(ex1, ey, 10, SSD1306_WHITE);
      display.fillCircle(ex2, ey, 10, SSD1306_WHITE);
      display.fillCircle(ex1, ey,  5, SSD1306_BLACK);
      display.fillCircle(ex2, ey,  5, SSD1306_BLACK);
      display.fillCircle(ex1-2, ey-2, 2, SSD1306_WHITE);
      display.fillCircle(ex2-2, ey-2, 2, SSD1306_WHITE);
      gfx_ellipse(mx, my+2, 10, 7, SSD1306_WHITE);
      gfx_ellipse(mx, my+2,  7, 4, SSD1306_BLACK);
      printLabel("!!!");
      break;

    case FACE_WALK:
      display.fillCircle(ex1, ey, 7, SSD1306_WHITE);
      display.fillCircle(ex2, ey, 7, SSD1306_WHITE);
      display.fillCircle(ex1+1, ey, 3, SSD1306_BLACK);
      display.fillCircle(ex2+1, ey, 3, SSD1306_BLACK);
      for (int i=-10; i<=10; i++)
        display.drawPixel(mx+i, my+(int)(0.02f*i*i), SSD1306_WHITE);
      display.drawLine(110,32,122,32, SSD1306_WHITE);
      display.drawLine(116,26,122,32, SSD1306_WHITE);
      display.drawLine(116,38,122,32, SSD1306_WHITE);
      printLabel("GO!");
      break;

    case FACE_TURN_L:
      display.fillCircle(ex1, ey, 7, SSD1306_WHITE);
      display.fillCircle(ex2, ey, 7, SSD1306_WHITE);
      display.fillCircle(ex1-2, ey, 3, SSD1306_BLACK);
      display.fillCircle(ex2-2, ey, 3, SSD1306_BLACK);
      display.drawLine(6,32,18,32, SSD1306_WHITE);
      display.drawLine(6,32,12,26, SSD1306_WHITE);
      display.drawLine(6,32,12,38, SSD1306_WHITE);
      display.drawLine(mx-8, my, mx+8, my, SSD1306_WHITE);
      printLabel("LEFT");
      break;

    case FACE_TURN_R:
      display.fillCircle(ex1, ey, 7, SSD1306_WHITE);
      display.fillCircle(ex2, ey, 7, SSD1306_WHITE);
      display.fillCircle(ex1+2, ey, 3, SSD1306_BLACK);
      display.fillCircle(ex2+2, ey, 3, SSD1306_BLACK);
      display.drawLine(110,32,122,32, SSD1306_WHITE);
      display.drawLine(116,26,122,32, SSD1306_WHITE);
      display.drawLine(116,38,122,32, SSD1306_WHITE);
      display.drawLine(mx-8, my, mx+8, my, SSD1306_WHITE);
      printLabel("RIGHT");
      break;

    default: // FACE_NEUTRAL
      display.fillCircle(ex1, ey, 7, SSD1306_WHITE);
      display.fillCircle(ex2, ey, 7, SSD1306_WHITE);
      display.fillCircle(ex1, ey, 3, SSD1306_BLACK);
      display.fillCircle(ex2, ey, 3, SSD1306_BLACK);
      display.drawLine(mx-8, my, mx+8, my, SSD1306_WHITE);
      printLabel("...");
      break;
  }
  display.display();
}

// ── Dimensioni robot ──────────────────────────────────────────
const float length_a    = 55;
const float length_b    = 77.5;
const float length_c    = 27.5;
const float length_side = 71;
const float z_absolute  = -28;

const float z_default = -50, z_up = -30, z_boot = z_absolute;
const float x_default = 62,  x_offset = 0;
const float y_start   = 0,   y_step   = 40;
const float y_default = x_default;

float site_now[4][3];
float site_expect[4][3];
float temp_speed[4][3];
float move_speed;
float speed_multiple = 1;

const float spot_turn_speed  = 4;
const float leg_move_speed   = 8;
const float body_move_speed  = 3;
const float stand_seat_speed = 1;
const float KEEP = 255;
const float pi   = 3.1415926;

unsigned long last_servo_ms = 0;
const unsigned long SERVO_INTERVAL = 20;

const float temp_a     = sqrt(pow(2*x_default+length_side,2)+pow(y_step,2));
const float temp_b     = 2*(y_start+y_step)+length_side;
const float temp_c_len = sqrt(pow(2*x_default+length_side,2)+pow(2*y_start+y_step+length_side,2));
const float temp_alpha = acos((pow(temp_a,2)+pow(temp_b,2)-pow(temp_c_len,2))/2/temp_a/temp_b);
const float turn_x1   = (temp_a-length_side)/2;
const float turn_y1   = y_start+y_step/2;
const float turn_x0   = turn_x1-temp_b*cos(temp_alpha);
const float turn_y0   = temp_b*sin(temp_alpha)-turn_y1-length_side;

// ── Prototipi ──────────────────────────────────────────────────
void readSerial(void);
void parseCommand(char*);
void executeMove(char);
void servo_service(void);
void set_site(int,float,float,float);
void wait_reach(int);
void wait_all_reach(void);
void cartesian_to_polar(float&,float&,float&,float,float,float);
void polar_to_servo(int,float,float,float);
bool is_stand(void);
void sit(void);
void stand(void);
void legs_init(void);
void body_left(int); void body_right(int);
void head_up(int);   void head_down(int);
void hand_wave(int); void hand_shake(int);
void step_forward(unsigned int);
void step_back(unsigned int);
void turn_left(unsigned int);
void turn_right(unsigned int);
void readSonars(void);
void ai_step(void);
void sendSensorData(void);

// ─────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  delay(500);
  Serial.println("=== Spider Robot Boot ===");

  // ── I2C scan: find OLED and PCA9685 ──────────────────────────
  Wire.begin();
  Serial.println("I2C scan...");
  bool found_oled = false;
  bool found_pca  = false;
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("  I2C device at 0x");
      if (addr < 16) Serial.print("0");
      Serial.println(addr, HEX);
      if (addr == 0x3C || addr == 0x3D) found_oled = true;
      if (addr == 0x40 || addr == 0x41) found_pca  = true;
    }
  }
  if (!found_oled) Serial.println("  WARNING: OLED not found on I2C!");
  if (!found_pca)  Serial.println("  WARNING: PCA9685 not found on I2C!");

  // ── OLED: try 0x3C first, then 0x3D ──────────────────────────
  bool oled_ok = display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  if (!oled_ok) {
    Serial.println("OLED 0x3C failed, trying 0x3D...");
    oled_ok = display.begin(SSD1306_SWITCHCAPVCC, 0x3D);
  }
  if (oled_ok) {
    Serial.println("OLED OK");
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(20,20); display.println("Spider Robot");
    display.setCursor(25,35); display.println("Starting...");
    display.display();
  } else {
    Serial.println("OLED ERROR: check wiring SDA=A4 SCL=A5");
  }

  // ── PCA9685 ───────────────────────────────────────────────────
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);
  delay(10);
  Serial.println("PCA9685 init done");

  // ── Sonar digital pins ────────────────────────────────────────
  pinMode(TRIG_FRONT, OUTPUT); pinMode(ECHO_FRONT, INPUT);
  pinMode(TRIG_LEFT,  OUTPUT); pinMode(ECHO_LEFT,  INPUT);
  pinMode(TRIG_RIGHT, OUTPUT); pinMode(ECHO_RIGHT, INPUT);
  Serial.println("Sonar pins OK");

  // ── Test sonar quickly ────────────────────────────────────────
  unsigned int tf = sonar_front.ping_cm();
  unsigned int tl = sonar_left.ping_cm();
  unsigned int tr = sonar_right.ping_cm();
  Serial.print("Sonar test: F="); Serial.print(tf ? tf : 99);
  Serial.print(" L="); Serial.print(tl ? tl : 99);
  Serial.print(" R="); Serial.println(tr ? tr : 99);

  // ── Servo initial positions ───────────────────────────────────
  set_site(0, x_default-x_offset, y_start+y_step, z_boot);
  set_site(1, x_default-x_offset, y_start+y_step, z_boot);
  set_site(2, x_default+x_offset, y_start,         z_boot);
  set_site(3, x_default+x_offset, y_start,         z_boot);
  for (int i=0;i<4;i++)
    for (int j=0;j<3;j++)
      site_now[i][j] = site_expect[i][j];

  delay(500);
  if (oled_ok) drawFace(FACE_HAPPY);
  Serial.println("READY");
}

// ─────────────────────────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  if (now - last_servo_ms >= SERVO_INTERVAL) {
    last_servo_ms = now;
    servo_service();
  }

  if (now - last_sonar_ms >= SONAR_INTERVAL) {
    last_sonar_ms = now;
    readSonars();
    sendSensorData();
  }

  readSerial();

  if (ai_mode) ai_step();
}

// ── Sonar ─────────────────────────────────────────────────────
void readSonars() {
  dist_front = sonar_front.ping_cm(); delay(15);
  dist_left  = sonar_left.ping_cm();  delay(15);
  dist_right = sonar_right.ping_cm();
  if (!dist_front) dist_front = 99;
  if (!dist_left)  dist_left  = 99;
  if (!dist_right) dist_right = 99;
}

void sendSensorData() {
  Serial.print("SONAR:");
  Serial.print(dist_front); Serial.print(",");
  Serial.print(dist_left);  Serial.print(",");
  Serial.println(dist_right);
}

// ── AI locale ─────────────────────────────────────────────────
unsigned long last_ai_ms = 0;
#define AI_INTERVAL 400

void ai_step() {
  if (millis() - last_ai_ms < AI_INTERVAL) return;
  last_ai_ms = millis();
  if (!is_stand()) stand();

  if (dist_front < DANGER_DIST) {
    drawFace(FACE_SCARED);
    delay(300);
    if (dist_left > dist_right) { drawFace(FACE_TURN_L); turn_left(3); }
    else                         { drawFace(FACE_TURN_R); turn_right(3); }
  } else if (dist_front < WARN_DIST) {
    if (dist_left > dist_right) { drawFace(FACE_TURN_L); turn_left(2); }
    else                         { drawFace(FACE_TURN_R); turn_right(2); }
  } else {
    drawFace(FACE_WALK);
    step_forward(2);
  }
}

// ── Serial ────────────────────────────────────────────────────
void readSerial() {
  static char buf[32];
  static uint8_t idx = 0;
  while (Serial.available()) {
    char c = Serial.read();
    if (c=='\n'||c=='\r') {
      if (idx>0) { buf[idx]='\0'; parseCommand(buf); idx=0; }
    } else if (idx<31) { buf[idx++]=c; }
  }
}

void parseCommand(char* line) {
  if (strcmp(line,"MODE_AI")==0) {
    ai_mode=true; drawFace(FACE_CUTE);
    Serial.println("ACK:MODE_AI"); return;
  }
  if (strcmp(line,"MODE_MANUAL")==0) {
    ai_mode=false; drawFace(FACE_NEUTRAL);
    Serial.println("ACK:MODE_MANUAL"); return;
  }
  if (strncmp(line,"FACE:",5)==0) {
    char* fn=line+5;
    if      (!strcmp(fn,"happy"))   drawFace(FACE_HAPPY);
    else if (!strcmp(fn,"sad"))     drawFace(FACE_SAD);
    else if (!strcmp(fn,"angry"))   drawFace(FACE_ANGRY);
    else if (!strcmp(fn,"annoyed")) drawFace(FACE_ANNOYED);
    else if (!strcmp(fn,"cute"))    drawFace(FACE_CUTE);
    else if (!strcmp(fn,"scared"))  drawFace(FACE_SCARED);
    else                             drawFace(FACE_NEUTRAL);
    return;
  }
  if (strlen(line)==1) { executeMove(line[0]); return; }
  Serial.println("ERR:unknown");
}

void executeMove(char cmd) {
  if (!is_stand()) stand();
  switch(cmd) {
    case 'F': drawFace(FACE_WALK);   step_forward(3); drawFace(FACE_NEUTRAL); break;
    case 'B': drawFace(FACE_ANNOYED);step_back(3);    drawFace(FACE_NEUTRAL); break;
    case 'L': drawFace(FACE_TURN_L); turn_left(3);    drawFace(FACE_NEUTRAL); break;
    case 'R': drawFace(FACE_TURN_R); turn_right(3);   drawFace(FACE_NEUTRAL); break;
    case 'S': drawFace(FACE_SAD);    sit();            break;
    default:  Serial.println("ERR:unknown_move");     break;
  }
}

// ── Servo service ─────────────────────────────────────────────
void servo_service(void) {
  float alpha,beta,gamma;
  for (int i=0;i<4;i++) {
    for (int j=0;j<3;j++) {
      if (abs(site_now[i][j]-site_expect[i][j])>=abs(temp_speed[i][j]))
        site_now[i][j]+=temp_speed[i][j];
      else
        site_now[i][j]=site_expect[i][j];
    }
    cartesian_to_polar(alpha,beta,gamma,
      site_now[i][0],site_now[i][1],site_now[i][2]);
    polar_to_servo(i,alpha,beta,gamma);
  }
}

void cartesian_to_polar(float &alpha,float &beta,float &gamma,
                        float x,float y,float z) {
  float v,w;
  w=((x>=0)?1:-1)*sqrt(x*x+y*y);
  v=w-length_c;
  alpha=atan2(z,v)+acos((length_a*length_a-length_b*length_b+v*v+z*z)
                         /2/length_a/sqrt(v*v+z*z));
  beta =acos((length_a*length_a+length_b*length_b-v*v-z*z)
              /2/length_a/length_b);
  gamma=(w>=0)?atan2(y,x):atan2(-y,-x);
  alpha=alpha/pi*180; beta=beta/pi*180; gamma=gamma/pi*180;
}

void polar_to_servo(int leg,float alpha,float beta,float gamma) {
  if      (leg==0){alpha=90-alpha;gamma+=90;}
  else if (leg==1){alpha+=90;beta=180-beta;gamma=90-gamma;}
  else if (leg==2){alpha+=90;beta=180-beta;gamma=90-gamma;}
  else if (leg==3){alpha=90-alpha;gamma+=90;}
  pwm_write(servo_channel[leg][0],alpha);
  pwm_write(servo_channel[leg][1],beta);
  pwm_write(servo_channel[leg][2],gamma);
}

// ── set_site / wait ────────────────────────────────────────────
void set_site(int leg,float x,float y,float z) {
  float lx=(x!=KEEP)?x-site_now[leg][0]:0;
  float ly=(y!=KEEP)?y-site_now[leg][1]:0;
  float lz=(z!=KEEP)?z-site_now[leg][2]:0;
  float len=sqrt(lx*lx+ly*ly+lz*lz);
  if(len==0)len=1;
  temp_speed[leg][0]=lx/len*move_speed*speed_multiple;
  temp_speed[leg][1]=ly/len*move_speed*speed_multiple;
  temp_speed[leg][2]=lz/len*move_speed*speed_multiple;
  if(x!=KEEP)site_expect[leg][0]=x;
  if(y!=KEEP)site_expect[leg][1]=y;
  if(z!=KEEP)site_expect[leg][2]=z;
}

void wait_reach(int leg) {
  while(site_now[leg][0]!=site_expect[leg][0]||
        site_now[leg][1]!=site_expect[leg][1]||
        site_now[leg][2]!=site_expect[leg][2]) {
    unsigned long n=millis();
    if(n-last_servo_ms>=SERVO_INTERVAL){last_servo_ms=n;servo_service();}
  }
}
void wait_all_reach(void){for(int i=0;i<4;i++)wait_reach(i);}

// ── Movimento base ─────────────────────────────────────────────
bool is_stand(void){return site_now[0][2]==z_default;}

void sit(void){
  move_speed=stand_seat_speed;
  for(int l=0;l<4;l++)set_site(l,KEEP,KEEP,z_boot);
  wait_all_reach();
}
void stand(void){
  move_speed=stand_seat_speed;
  for(int l=0;l<4;l++)set_site(l,KEEP,KEEP,z_default);
  wait_all_reach();
}
void legs_init(void){
  move_speed=8;
  for(int l=0;l<4;l++)set_site(l,KEEP,0,90);
  wait_all_reach();
}

void body_left(int i){
  set_site(0,site_now[0][0]+i,KEEP,KEEP);set_site(1,site_now[1][0]+i,KEEP,KEEP);
  set_site(2,site_now[2][0]-i,KEEP,KEEP);set_site(3,site_now[3][0]-i,KEEP,KEEP);
  wait_all_reach();
}
void body_right(int i){
  set_site(0,site_now[0][0]-i,KEEP,KEEP);set_site(1,site_now[1][0]-i,KEEP,KEEP);
  set_site(2,site_now[2][0]+i,KEEP,KEEP);set_site(3,site_now[3][0]+i,KEEP,KEEP);
  wait_all_reach();
}
void head_up(int i){
  set_site(0,KEEP,KEEP,site_now[0][2]-i);set_site(1,KEEP,KEEP,site_now[1][2]+i);
  set_site(2,KEEP,KEEP,site_now[2][2]-i);set_site(3,KEEP,KEEP,site_now[3][2]+i);
  wait_all_reach();
}
void head_down(int i){
  set_site(0,KEEP,KEEP,site_now[0][2]+i);set_site(1,KEEP,KEEP,site_now[1][2]-i);
  set_site(2,KEEP,KEEP,site_now[2][2]+i);set_site(3,KEEP,KEEP,site_now[3][2]-i);
  wait_all_reach();
}

void hand_wave(int i){
  float xt,yt,zt; move_speed=1;
  if(site_now[3][1]==y_start){
    body_right(15);
    xt=site_now[2][0];yt=site_now[2][1];zt=site_now[2][2];
    move_speed=body_move_speed;
    for(int j=0;j<i;j++){
      set_site(2,turn_x1,turn_y1,50);wait_all_reach();
      set_site(2,turn_x0,turn_y0,50);wait_all_reach();
    }
    set_site(2,xt,yt,zt);wait_all_reach();
    move_speed=1;body_left(15);
  }else{
    body_left(15);
    xt=site_now[0][0];yt=site_now[0][1];zt=site_now[0][2];
    move_speed=body_move_speed;
    for(int j=0;j<i;j++){
      set_site(0,turn_x1,turn_y1,50);wait_all_reach();
      set_site(0,turn_x0,turn_y0,50);wait_all_reach();
    }
    set_site(0,xt,yt,zt);wait_all_reach();
    move_speed=1;body_right(15);
  }
}

void hand_shake(int i){
  float xt,yt,zt; move_speed=1;
  if(site_now[3][1]==y_start){
    body_right(15);
    xt=site_now[2][0];yt=site_now[2][1];zt=site_now[2][2];
    move_speed=body_move_speed;
    for(int j=0;j<i;j++){
      set_site(2,x_default-30,y_start+2*y_step,55);wait_all_reach();
      set_site(2,x_default-30,y_start+2*y_step,10);wait_all_reach();
    }
    set_site(2,xt,yt,zt);wait_all_reach();
    move_speed=1;body_left(15);
  }else{
    body_left(15);
    xt=site_now[0][0];yt=site_now[0][1];zt=site_now[0][2];
    move_speed=body_move_speed;
    for(int j=0;j<i;j++){
      set_site(0,x_default-30,y_start+2*y_step,55);wait_all_reach();
      set_site(0,x_default-30,y_start+2*y_step,10);wait_all_reach();
    }
    set_site(0,xt,yt,zt);wait_all_reach();
    move_speed=1;body_right(15);
  }
}

void step_forward(unsigned int step){
  move_speed=leg_move_speed;
  while(step-->0){
    if(site_now[2][1]==y_start){
      set_site(2,x_default+x_offset,y_start,          z_up);      wait_all_reach();
      set_site(2,x_default+x_offset,y_start+2*y_step, z_up);      wait_all_reach();
      set_site(2,x_default+x_offset,y_start+2*y_step, z_default); wait_all_reach();
      move_speed=body_move_speed;
      set_site(0,x_default+x_offset,y_start,          z_default);
      set_site(1,x_default+x_offset,y_start+2*y_step, z_default);
      set_site(2,x_default-x_offset,y_start+y_step,   z_default);
      set_site(3,x_default-x_offset,y_start+y_step,   z_default);
      wait_all_reach();
      move_speed=leg_move_speed;
      set_site(1,x_default+x_offset,y_start+2*y_step, z_up);      wait_all_reach();
      set_site(1,x_default+x_offset,y_start,          z_up);      wait_all_reach();
      set_site(1,x_default+x_offset,y_start,          z_default); wait_all_reach();
    }else{
      set_site(0,x_default+x_offset,y_start,          z_up);      wait_all_reach();
      set_site(0,x_default+x_offset,y_start+2*y_step, z_up);      wait_all_reach();
      set_site(0,x_default+x_offset,y_start+2*y_step, z_default); wait_all_reach();
      move_speed=body_move_speed;
      set_site(0,x_default-x_offset,y_start+y_step,   z_default);
      set_site(1,x_default-x_offset,y_start+y_step,   z_default);
      set_site(2,x_default+x_offset,y_start,          z_default);
      set_site(3,x_default+x_offset,y_start+2*y_step, z_default);
      wait_all_reach();
      move_speed=leg_move_speed;
      set_site(3,x_default+x_offset,y_start+2*y_step, z_up);      wait_all_reach();
      set_site(3,x_default+x_offset,y_start,          z_up);      wait_all_reach();
      set_site(3,x_default+x_offset,y_start,          z_default); wait_all_reach();
    }
  }
}

void step_back(unsigned int step){
  move_speed=leg_move_speed;
  while(step-->0){
    if(site_now[3][1]==y_start){
      set_site(3,x_default+x_offset,y_start,          z_up);      wait_all_reach();
      set_site(3,x_default+x_offset,y_start+2*y_step, z_up);      wait_all_reach();
      set_site(3,x_default+x_offset,y_start+2*y_step, z_default); wait_all_reach();
      move_speed=body_move_speed;
      set_site(0,x_default+x_offset,y_start+2*y_step, z_default);
      set_site(1,x_default+x_offset,y_start,          z_default);
      set_site(2,x_default-x_offset,y_start+y_step,   z_default);
      set_site(3,x_default-x_offset,y_start+y_step,   z_default);
      wait_all_reach();
      move_speed=leg_move_speed;
      set_site(0,x_default+x_offset,y_start+2*y_step, z_up);      wait_all_reach();
      set_site(0,x_default+x_offset,y_start,          z_up);      wait_all_reach();
      set_site(0,x_default+x_offset,y_start,          z_default); wait_all_reach();
    }else{
      set_site(1,x_default+x_offset,y_start,          z_up);      wait_all_reach();
      set_site(1,x_default+x_offset,y_start+2*y_step, z_up);      wait_all_reach();
      set_site(1,x_default+x_offset,y_start+2*y_step, z_default); wait_all_reach();
      move_speed=body_move_speed;
      set_site(0,x_default-x_offset,y_start+y_step,   z_default);
      set_site(1,x_default-x_offset,y_start+y_step,   z_default);
      set_site(2,x_default+x_offset,y_start+2*y_step, z_default);
      set_site(3,x_default+x_offset,y_start,          z_default);
      wait_all_reach();
      move_speed=leg_move_speed;
      set_site(2,x_default+x_offset,y_start+2*y_step, z_up);      wait_all_reach();
      set_site(2,x_default+x_offset,y_start,          z_up);      wait_all_reach();
      set_site(2,x_default+x_offset,y_start,          z_default); wait_all_reach();
    }
  }
}

void turn_left(unsigned int step){
  move_speed=spot_turn_speed;
  while(step-->0){
    if(site_now[3][1]==y_start){
      set_site(3,x_default+x_offset,y_start,z_up);wait_all_reach();
      set_site(0,turn_x1-x_offset,turn_y1,z_default);
      set_site(1,turn_x0-x_offset,turn_y0,z_default);
      set_site(2,turn_x1+x_offset,turn_y1,z_default);
      set_site(3,turn_x0+x_offset,turn_y0,z_up);
      wait_all_reach();
      set_site(3,turn_x0+x_offset,turn_y0,z_default);wait_all_reach();
      set_site(0,turn_x1+x_offset,turn_y1,z_default);
      set_site(1,turn_x0+x_offset,turn_y0,z_default);
      set_site(2,turn_x1-x_offset,turn_y1,z_default);
      set_site(3,turn_x0-x_offset,turn_y0,z_default);
      wait_all_reach();
      set_site(1,turn_x0+x_offset,turn_y0,z_up);wait_all_reach();
      set_site(0,x_default+x_offset,y_start,       z_default);
      set_site(1,x_default+x_offset,y_start,       z_up);
      set_site(2,x_default-x_offset,y_start+y_step,z_default);
      set_site(3,x_default-x_offset,y_start+y_step,z_default);
      wait_all_reach();
      set_site(1,x_default+x_offset,y_start,z_default);wait_all_reach();
    }else{
      set_site(0,x_default+x_offset,y_start,z_up);wait_all_reach();
      set_site(0,turn_x0+x_offset,turn_y0,z_up);
      set_site(1,turn_x1+x_offset,turn_y1,z_default);
      set_site(2,turn_x0-x_offset,turn_y0,z_default);
      set_site(3,turn_x1-x_offset,turn_y1,z_default);
      wait_all_reach();
      set_site(0,turn_x0+x_offset,turn_y0,z_default);wait_all_reach();
      set_site(0,turn_x0-x_offset,turn_y0,z_default);
      set_site(1,turn_x1-x_offset,turn_y1,z_default);
      set_site(2,turn_x0+x_offset,turn_y0,z_default);
      set_site(3,turn_x1+x_offset,turn_y1,z_default);
      wait_all_reach();
      set_site(2,turn_x0+x_offset,turn_y0,z_up);wait_all_reach();
      set_site(0,x_default-x_offset,y_start+y_step,z_default);
      set_site(1,x_default-x_offset,y_start+y_step,z_default);
      set_site(2,x_default+x_offset,y_start,       z_up);
      set_site(3,x_default+x_offset,y_start,       z_default);
      wait_all_reach();
      set_site(2,x_default+x_offset,y_start,z_default);wait_all_reach();
    }
  }
}

void turn_right(unsigned int step){
  move_speed=spot_turn_speed;
  while(step-->0){
    if(site_now[2][1]==y_start){
      set_site(2,x_default+x_offset,y_start,z_up);wait_all_reach();
      set_site(0,turn_x0-x_offset,turn_y0,z_default);
      set_site(1,turn_x1-x_offset,turn_y1,z_default);
      set_site(2,turn_x0+x_offset,turn_y0,z_up);
      set_site(3,turn_x1+x_offset,turn_y1,z_default);
      wait_all_reach();
      set_site(2,turn_x0+x_offset,turn_y0,z_default);wait_all_reach();
      set_site(0,turn_x0+x_offset,turn_y0,z_default);
      set_site(1,turn_x1+x_offset,turn_y1,z_default);
      set_site(2,turn_x0-x_offset,turn_y0,z_default);
      set_site(3,turn_x1-x_offset,turn_y1,z_default);
      wait_all_reach();
      set_site(0,turn_x0+x_offset,turn_y0,z_up);wait_all_reach();
      set_site(0,x_default+x_offset,y_start,       z_up);
      set_site(1,x_default+x_offset,y_start,       z_default);
      set_site(2,x_default-x_offset,y_start+y_step,z_default);
      set_site(3,x_default-x_offset,y_start+y_step,z_default);
      wait_all_reach();
      set_site(0,x_default+x_offset,y_start,z_default);wait_all_reach();
    }else{
      set_site(1,x_default+x_offset,y_start,z_up);wait_all_reach();
      set_site(0,turn_x1+x_offset,turn_y1,z_default);
      set_site(1,turn_x0+x_offset,turn_y0,z_up);
      set_site(2,turn_x1-x_offset,turn_y1,z_default);
      set_site(3,turn_x0-x_offset,turn_y0,z_default);
      wait_all_reach();
      set_site(1,turn_x0+x_offset,turn_y0,z_default);wait_all_reach();
      set_site(0,turn_x1-x_offset,turn_y1,z_default);
      set_site(1,turn_x0-x_offset,turn_y0,z_default);
      set_site(2,turn_x1+x_offset,turn_y1,z_default);
      set_site(3,turn_x0+x_offset,turn_y0,z_default);
      wait_all_reach();
      set_site(3,turn_x0+x_offset,turn_y0,z_up);wait_all_reach();
      set_site(0,x_default-x_offset,y_start+y_step,z_default);
      set_site(1,x_default-x_offset,y_start+y_step,z_default);
      set_site(2,x_default+x_offset,y_start,       z_default);
      set_site(3,x_default+x_offset,y_start,       z_up);
      wait_all_reach();
      set_site(3,x_default+x_offset,y_start,z_default);wait_all_reach();
    }
  }
}
