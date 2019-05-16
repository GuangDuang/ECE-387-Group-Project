#include <SoftwareSerial.h>
#include <SPI.h>
#include <SD.h>
#include <Servo.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_TFTLCD.h>
#include <Adafruit_GFX.h>
#include <TouchScreen.h>
//camera & SD
File myFile,imgFile;
int picCnt = 0;

//Camera
byte incomingbyte;
SoftwareSerial cameraSerial = SoftwareSerial(13, 12);   //Configure pin 2 and 3 as soft serial port
int a = 0x0000, j = 0, k = 0, count = 0; //Read Starting address
uint8_t MH, ML;
boolean EndFlag = 0,Open=false;
char txtTitle[25] = {},imgTitle[25] = {};


//Screen
int income = 0;
int addr = 0;
int pointer = 0;
int compare = 0;
int qrlength = 0;
int flag = -1;
int matchflag = 0;
int firstcount = 0;
String pin;
String password;
boolean scan = false;
boolean id = false;
boolean locked = false;

int tonePin = 22;
int lockIndecate = 11;

#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
#define LCD_RESET A4

#define TS_MINX 122
#define TS_MINY 111
#define TS_MAXX 942
#define TS_MAXY 890

#define YP A3
#define XM A2
#define YM 9
#define XP 8

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define GREY      0xCE79
#define LIGHTGREY 0xDEDB

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

int passwords[4];
int i = 0;
boolean enter = false;
boolean one = false, two = false;

Servo myservo;

void setup() {
  Serial3.begin(9600);
  Serial.begin(9600);
  pinMode(lockIndecate, OUTPUT);
  cameraSerial.begin(38400);
  SendResetCmd();
  checkSD();
  tft.reset();
  uint16_t identifier = tft.readID();
  tft.begin(identifier);
  lock();
  qrlength = 0;

  sizecheck();
}
//==============================loop=============================================
void loop() {
  if (!id) {
    scann();
  } else if (!locked) {
    waitLock();
  } else {
    waitUnlock();
  }
}
//==============================END Loop=============================================
//===========================waitUnlock=============================================
void waitUnlock() {
  Serial.println("enter to unlock");
  secondpage();
  while (true) {
    check();
    while (Serial.available() > 0) {
      pin += Serial.read();
      Serial.println(pin);
    }
    if (pin.length() == 4) {
      if (password.equals(pin)) {
        password = "";
        pin = "";
        unlock();
        locked = false;
        id = false;
        delay(10000);
        return;
      } else {
        pin = "";
        retry();
      }
    }else if (pin.length() > 4)pin = "";
    else checkbut();
    noTone(tonePin);
  }
}
//===========================END waitUnlock=============================================
//============================waitlock===============================================
void waitLock() {
  Serial.println("enter to lock");
  secondpage();
  while (true) {
    check();
    while (Serial.available() > 0) {
      pin += Serial.read();
      Serial.println(pin);
    }
    if (pin.length() == 4) {
      password = pin;
      Serial.println(pin);
      pin = "";
      lock();
      locked = true;
      return;
    }else if (pin.length() > 4)pin = "";
  }

}
//===========================END waitlock===============================================
//==============================Scan=================================================
void scann() {
  Serial.println("check id");
  lock();
  firstpage();
  while (true) {

    while (Serial3.available() > 0) {
      // display each character to the LCD
      scan = true;
      income = Serial3.read();
      checking(income);
    }
    if (scan == true && income == 13) {
      checkId();
      return;
    }
  }
}
//==============================END Scan============================================
//==============================checkId==============================================
void checkId() {
  Serial.println("checking...");
  if (matchflag != qrlength) {
    Serial.println("Try again");
    error();
    delay(3000);
    pointer = 0;
    matchflag = 0;
    Serial.println(qrlength);
    scan = false;
    return;
  }
  if (matchflag == qrlength) {
    Serial.println("Pass!");
    approve();
    pointer = 0;
    matchflag = 0;
    id = true;
    scan = false;
    unlock();
    delay(5000);
    return;
  }
}
//=============================END checkId===========================================
//===================================================================================
//                                SD Camera
//===================================================================================
//=============================Save==================================================
void save(){
  sprintf(txtTitle, "pic%d.txt", picCnt);
  myFile = SD.open(txtTitle,FILE_WRITE);

  while (cameraSerial.available() > 0) {
    incomingbyte = cameraSerial.read(); //clear unneccessary serial from camera
  }
  byte b[32];
  Serial.println("Saving the picture as txt file...");
  Serial.println("This might takes a few sec..");
  while (!EndFlag) {
    Serial.println(1);
    j = 0;
    k = 0;
    count = 0;
    SendReadDataCmd(); //command to get picture from camera
    delay(75); //delay necessary for data not to be lost
    while (cameraSerial.available() > 0) {
      Serial.println(2);
      incomingbyte = cameraSerial.read(); //read serial from camer
      k++;
      if ((k > 5) && (j < 32) && (!EndFlag)) {
        b[j] = incomingbyte;
        if ((b[j - 1] == 0xFF) && (b[j] == 0xD9))
          EndFlag = 1; //when end of picture appears, stop reading data
        j++;
        count++;
      }
    }

    for (j = 0; j < count; j++) { //store picture into file
      if (b[j] < 0x10)
        myFile.print("0");
        myFile.print(b[j], HEX);
    }
    myFile.println();
  }

  StopTakePhotoCmd(); //stop this picture so another one can be taken
  EndFlag = 0; // reset flag to allow another picture to be read
  Serial.println("Immage is saved!");
  Serial.println();
  myFile.close(); //close file
  picCnt++; //increment value for next picture
  Open = false;
}


//=========================END Save==================================================
//Send Reset command
void SendResetCmd() {
  cameraSerial.write((byte)0x56);
  cameraSerial.write((byte)0x00);
  cameraSerial.write((byte)0x26);
  cameraSerial.write((byte)0x00);
}

//Send take picture command
void SendTakePhotoCmd() {
  cameraSerial.write((byte)0x56);
  cameraSerial.write((byte)0x00);
  cameraSerial.write((byte)0x36);
  cameraSerial.write((byte)0x01);
  cameraSerial.write((byte)0x00);

  a = 0x0000; //reset so that another picture can taken
  save();
}

void FrameSize() {
  cameraSerial.write((byte)0x56);
  cameraSerial.write((byte)0x00);
  cameraSerial.write((byte)0x34);
  cameraSerial.write((byte)0x01);
  cameraSerial.write((byte)0x00);
}

//Read data
void SendReadDataCmd() {
  MH = a / 0x100;
  ML = a % 0x100;

  cameraSerial.write((byte)0x56);
  cameraSerial.write((byte)0x00);
  cameraSerial.write((byte)0x32);
  cameraSerial.write((byte)0x0c);
  cameraSerial.write((byte)0x00);
  cameraSerial.write((byte)0x0a);
  cameraSerial.write((byte)0x00);
  cameraSerial.write((byte)0x00);
  cameraSerial.write((byte)MH);
  cameraSerial.write((byte)ML);
  cameraSerial.write((byte)0x00);
  cameraSerial.write((byte)0x00);
  cameraSerial.write((byte)0x00);
  cameraSerial.write((byte)0x20);
  cameraSerial.write((byte)0x00);
  cameraSerial.write((byte)0x0a);

  a += 0x20;
  Serial.println(0);
}

void StopTakePhotoCmd() {
  cameraSerial.write((byte)0x56);
  cameraSerial.write((byte)0x00);
  cameraSerial.write((byte)0x36);
  cameraSerial.write((byte)0x01);
  cameraSerial.write((byte)0x03);
}

void checkbut(){
  Serial.println(analogRead(A5));
  if(analogRead(A5)>90){
    Open=true;
    tone(tonePin, 500);
    SendTakePhotoCmd();
  }
}

void checkSD(){
  if(!SD.begin(53)){
    Serial.println("initialization failed!");
    delay(500);
    checkSD();
  }
  else{
    Serial.println("initialization done."); 
    Serial.println("The SD card is ready..."); 
  }
}
//===================================================================================
//                                TOUCH SCREEN
//===================================================================================
//**********************************BUTTON*****************************************
void button() {
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  while (true) {
    TSPoint p = ts.getPoint();
    if (p.z > ts.pressureThreshhold) {
      p.x = map(p.x, TS_MAXX, TS_MINX, 0, 320);
      p.y = map(p.y, TS_MAXY, TS_MINY, 0, 480);

      if (p.y > 2 && p.y < 118 ) {
        if ( p.x > 2 && p.x < 106) {
          Serial.print("1");
          passwords[i] = 1;
          i++;
          sound();

        }
        else if ( p.x > 108 && p.x < 212) {
          Serial.print("2");
          passwords[i] = 2;
          i++;
          sound();
        }
        else if ( p.x > 214 && p.x < 318) {
          Serial.print("3");
          passwords[i] = 3;
          i++;
          sound();
        }
      }

      else if (p.y > 120 && p.y < 238 ) {
        if ( p.x > 2 && p.x < 106) {
          Serial.print("4");
          passwords[i] = 4;
          i++;
          sound();
        }
        else if ( p.x > 108 && p.x < 212) {
          Serial.print("5");
          passwords[i] = 5;
          i++;
          sound();
        }
        else if ( p.x > 214 && p.x < 318) {
          Serial.print("6");
          passwords[i] = 6;
          i++;
          sound();
        }
      }

      else if (p.y > 240 && p.y < 358 ) {
        if ( p.x > 2 && p.x < 106) {
          Serial.print("7");
          passwords[i] = 7;
          i++;
          sound();
        }
        else if ( p.x > 108 && p.x < 212) {
          Serial.print("8");
          passwords[i] = 8;
          i++;
          sound();
        }
        else if ( p.x > 214 && p.x < 318) {
          Serial.print("9");
          passwords[i] = 9;
          i++;
          sound();
        }
      }

      else if (p.y > 360 && p.y < 478 ) {
        if ( p.x > 2 && p.x < 106) {
          Serial.print("Clear");
          i = 0;
          int passwords[4];
          pin = "";
          enter = false;
          sound();
        }
        else if ( p.x > 108 && p.x < 212) {
          Serial.print("0");
          passwords[i] = 0;
          i++;
          sound();
        }
        else if ( p.x > 214 && p.x < 318) {
          Serial.print("Enter");
          enter = true;
          sound();
        }
      }
    }
     if (i != 4 && enter) {
      int passwords[4];
      i = 0;
      enter = false;
      retry();
      return;
    }
    if (i == 4 && enter) {
      for (i = 0; i < 4; i++) {
        Serial.print(passwords[i]);
        pin += passwords[i];
      }
      int passwords[4];
      i = 0;
      enter = false;
      return;
    }
  }
}
//========================================end button============================================
//=======================================keypad==================================================
void keypad() {
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  tft.setRotation(1);
  tft.fillScreen(BLACK);
  tft.drawRect(0, 0, 319, 240, BLACK);

  tft.fillRect(3, 3, 80, 80, YELLOW);
  tft.drawRect(3, 3, 80, 80, BLACK);
  tft.setCursor(30, 30);
  tft.setTextColor(BLACK);
  tft.setTextSize(5);
  tft.print("1");
  tft.fillRect(3, 80, 80, 80, YELLOW);
  tft.drawRect(3, 80, 80, 80, BLACK);
  tft.setCursor(30, 100);
  tft.setTextColor(BLACK);
  tft.setTextSize(5);
  tft.print("2");
  tft.fillRect(3, 160, 80, 77, YELLOW);
  tft.drawRect(3, 160, 80, 77, BLACK);
  tft.setCursor(30, 180);
  tft.setTextColor(BLACK);
  tft.setTextSize(5);
  tft.print("3");
  tft.fillRect(80, 3, 80, 80, YELLOW);
  tft.drawRect(80, 3, 80, 80, BLACK);
  tft.setCursor(105, 30);
  tft.setTextColor(BLACK);
  tft.setTextSize(5);
  tft.print("4");
  tft.fillRect(80, 80, 80, 80, YELLOW);
  tft.drawRect(80, 80, 80, 80, BLACK);
  tft.setCursor(105, 100);
  tft.setTextColor(BLACK);
  tft.setTextSize(5);
  tft.print("5");
  tft.fillRect(80, 160, 80, 77, YELLOW);
  tft.drawRect(80, 160, 80, 77, BLACK);
  tft.setCursor(105, 180);
  tft.setTextColor(BLACK);
  tft.setTextSize(5);
  tft.print("6");
  tft.fillRect(160, 3, 80, 80, YELLOW);
  tft.drawRect(160, 3, 80, 80, BLACK);
  tft.setCursor(185, 30);
  tft.setTextColor(BLACK);
  tft.setTextSize(5);
  tft.print("7");
  tft.fillRect(160, 80, 80, 80, YELLOW);
  tft.drawRect(160, 80, 80, 80, BLACK);
  tft.setCursor(185, 100);
  tft.setTextColor(BLACK);
  tft.setTextSize(5);
  tft.print("8");
  tft.fillRect(160, 160, 80, 77, YELLOW);
  tft.drawRect(160, 160, 80, 77, BLACK);
  tft.setCursor(185, 180);
  tft.setTextColor(BLACK);
  tft.setTextSize(5);
  tft.print("9");
  tft.fillRect(240, 3, 77, 80, RED);
  tft.drawRect(240, 3, 77, 80, BLACK);
  tft.setCursor(250, 40);
  tft.setTextColor(BLACK);
  tft.setTextSize(2);
  tft.print("CLEAR");
  tft.fillRect(240, 80, 77, 80, YELLOW);
  tft.drawRect(240, 80, 77, 80, BLACK);
  tft.setCursor(265, 100);
  tft.setTextColor(BLACK);
  tft.setTextSize(5);
  tft.print("0");
  tft.fillRect(240, 160, 77, 77, GREEN);
  tft.drawRect(240, 160, 77, 77, BLACK);
  tft.setCursor(250, 190);
  tft.setTextColor(BLACK);
  tft.setTextSize(2);
  tft.print("ENTER");
}
//========================================END keyboard===============================================

//========================================firstpage==================================================
void firstpage() {
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  tft.setRotation(1);
  tft.fillScreen(WHITE);
  tft.drawRect(0, 0, 319, 240, BLACK);

  tft.setCursor(50, 40);
  tft.setTextColor(BLACK);
  tft.setTextSize(3.5);
  tft.print(" Welcome!!! \n  Please verify \n   your identity");

  tft.setCursor(115, 150);
  tft.setTextColor(BLUE);
  tft.setTextSize(3);
  tft.print("*^_^*");
}
//=======================================End Firstpage================================================

//=======================================secondpage====================================================
void secondpage() {
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  tft.setRotation(1);
  tft.fillScreen(WHITE);
  tft.drawRect(0, 0, 319, 240, BLACK);

  tft.setCursor(50, 40);
  tft.setTextColor(BLACK);
  tft.setTextSize(3);
  if(locked){
    tft.print("Please enter \n  4 digital pin \n  to unlock");
  }
  else{
    tft.print("Please enter \n  4 digital pin \n     to lock");
  }
  tft.fillRect(70, 160, 180, 70, YELLOW);
  tft.drawRect(70, 160, 180, 70, YELLOW);
  tft.setCursor(90, 185);
  tft.setTextColor(BLACK);
  tft.setTextSize(2);
  tft.print("Enter Button");
}

void check() {
  TSPoint p = ts.getPoint();
  if (p.z > ts.pressureThreshhold) {

    p.x = map(p.x, TS_MAXX, TS_MINX, 0, 320);
    p.y = map(p.y, TS_MAXY, TS_MINY, 0, 480);
    if (p.y > 120 && p.y < 360 && p.x > 215 && p.x < 315) {
      keypad();
      delay(1000);
      button();
      secondpage();
      return;
    }
  }
}
//=======================================END secondpage====================================================

//=======================================approve====================================================
void approve() {
  TSPoint p = ts.getPoint();
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  tft.setRotation(1);
  tft.fillScreen(WHITE);
  tft.drawRect(0, 0, 319, 240, BLACK);

  tft.setCursor(40, 40);
  tft.setTextColor(BLACK);
  tft.setTextSize(3);
  tft.print("Lock has \n  been opened");

  tft.setCursor(40, 100);
  tft.setTextColor(BLUE);
  tft.setTextSize(3);
  tft.print("You can \n  use it now");

  tft.fillRect(70, 160, 180, 70, YELLOW);
  tft.drawRect(70, 160, 180, 70, YELLOW);
  tft.setCursor(90, 185);
  tft.setTextColor(BLACK);
  tft.setTextSize(2);
  tft.print("Unblock ");
}

//*****************************************retry*************************************
void retry() {
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  tft.setRotation(1);
  tft.fillScreen(WHITE);
  tft.drawRect(0, 0, 319, 240, BLACK);

  tft.setCursor(70, 40);
  tft.setTextColor(BLACK);
  tft.setTextSize(4);
  tft.print("Sorry");

  tft.setCursor(115, 120);
  tft.setTextColor(BLUE);
  tft.setTextSize(2);
  tft.print("Wrong information, please touch anywherer to enter password again");
  delay(1000);
  while (true) {
    TSPoint p = ts.getPoint();
    if (p.z > ts.pressureThreshhold) {
      return;
    }
  }
}

//********************************************error**********************************
void error() {
  TSPoint p = ts.getPoint();
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  tft.setRotation(1);
  tft.fillScreen(WHITE);
  tft.drawRect(0, 0, 319, 240, BLACK);

  tft.setCursor(55, 40);
  tft.setTextColor(BLACK);
  tft.setTextSize(4);
  tft.print("You've tried it three times");

  tft.setCursor(115, 120);
  tft.setTextColor(BLUE);
  tft.setTextSize(3);
  tft.print("Please verify your identity again");
}
//===================================================================================
//                                    OTHER
//===================================================================================
void unlock() {
  myservo.attach(10);
  myservo.write(50);
  digitalWrite(lockIndecate, LOW);
  delay(1000);
  myservo.detach();
}
void lock() {
  myservo.attach(10);
  myservo.write(140);
  digitalWrite(lockIndecate, HIGH);
  delay(1000);
  myservo.detach();
}
void sizecheck() {
  int i = -1;
  int arraypointer = 0;
  while (i != 0) {
    i = EEPROM.read(arraypointer);
    if (i != 0) {
      qrlength++;
      arraypointer++;
    }
  }
}
void checking(int input) {
  compare = EEPROM.read(pointer);
  if (compare == 0) {

    EEPROM.write(pointer, input);


    Serial.println(EEPROM.read(pointer));

    pointer++;

    qrlength++;

    firstcount++;

    return;
  }


  else if (input != compare && compare != 0) {

    pointer = 0;
    return;
  }
  else  if (input == compare) {

    pointer++;

    matchflag++;

    return;
  }
}
void sound(){
  tone(tonePin, 500);
  delay(300);
  noTone(tonePin);
}
