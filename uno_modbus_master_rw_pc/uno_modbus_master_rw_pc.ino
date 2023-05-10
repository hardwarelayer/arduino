/**************************************************************************
  Using Adafruit GFX and ST7735 library and example source codes
  Based on this: https://github.com/hardwarelayer/arduino_uno_generic_projects/tree/master/uno_st7735_test1
 **************************************************************************/

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>

#include <ModbusMaster.h>
#include <SoftwareSerial.h>

SoftwareSerial mySerial(10, 11); // RX, TX

ModbusMaster node;

#define MAX485_DE 2 //DE&RE share same connection
#define MAX485_RE_NEG 2

#define TFT_CS   A5
#define TFT_RST  A4  // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC   A3
#define TFT_MOSI A2  // Data out
#define TFT_SCLK A1  // Clock out
#define TFT_LED  A0

//skipping death pixels
#define SCREEN_START_Y 45

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

float p = 3.1415926;

void drawText(int y, char *text, uint16_t color) {
  tft.setCursor(0, SCREEN_START_Y+y);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}

void print(int y, char *text) {
  drawText(y, text, ST77XX_WHITE);
}

void preTransmission()
{
  digitalWrite(MAX485_RE_NEG, 1);
  digitalWrite(MAX485_DE, 1);
}

void postTransmission()
{
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
}

void setup(void) {

  pinMode(MAX485_RE_NEG, OUTPUT);
  //pinMode(MAX485_DE, OUTPUT);
  
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH);

  tft.initR(INITR_BLACKTAB);      // Init ST7735S chip, black tab

  uint16_t time = millis();
  tft.fillScreen(ST77XX_BLACK);
  time = millis() - time;

  print(0, "Start UNO Modbus master");
  delay(1000);

  Serial.begin(9600);
  
  mySerial.begin(9600);
  node.begin(1, mySerial);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
}


static float get_current_value() {
  float curVal = 0.0;

  uint16_t data[2];

  String myMsgO = " ... ";

  //read 2x16bits registers starting at register 3
  uint8_t result = node.readHoldingRegisters(0x40003, 2);

  myMsgO.concat(result); //arduino not support sprintf float
  myMsgO.concat(":");

  if (result == node.ku8MBSuccess) {
    for (int j = 0; j < 2; j++)
    {
      data[j] = node.getResponseBuffer(j);
      myMsgO.concat(data[j]); //arduino not support sprintf float
      myMsgO.concat(" ");

      
    }
    memcpy(&curVal, data, 2);
  }

  char myMsg[30];
  myMsgO.toCharArray(myMsg, myMsgO.length() + 1);
  print(50, myMsg);

  return curVal;
}

bool isOdd = false;

void loop() {

  static uint32_t i;
  i++;
   
  // set word 0 of TX buffer to least-significant word of counter (bits 15..0)
  node.setTransmitBuffer(0, lowWord(i));
  
  // set word 1 of TX buffer to most-significant word of counter (bits 31..16)
  node.setTransmitBuffer(1, highWord(i));

  tft.fillScreen(ST77XX_BLACK);

  if (isOdd)
    print(10, "WSR: 0x40000=01");
  else
    print(10, "WSR: 0x40000=00");

  if (isOdd)
    node.writeSingleRegister(0x40000,1);               //Writes 1 to 0x40000 holding register
   else
     node.writeSingleRegister(0x40000,0);

  delay(500);
  if (isOdd)
    print(20, "WSR: 0x40001=01");
  else
    print(20, "WSR: 0x40001=00");

  if (isOdd)
    node.writeSingleRegister(0x40001,1);               //Writes 1 to 0x40001 holding register
   else
     node.writeSingleRegister(0x40001,0);

  delay(500);
  if (isOdd)
    print(30, "WSR: 0x40002=88");
  else
    print(30, "WSR: 0x40002=66");

  if (isOdd)
    node.writeSingleRegister(0x40002,88);               //Writes 1 to 0x40001 holding register
   else
     node.writeSingleRegister(0x40002,66);

  print(40, "RSR: 0x40003 ...");
  float curVal = get_current_value();
  String myMsgO = "Read value ";
  myMsgO.concat(curVal); //arduino not support sprintf float
  char myMsg[20];
  myMsgO.toCharArray(myMsg, myMsgO.length() + 1);
  Serial.println(myMsg);
  print(60, myMsg);
  
  isOdd = !isOdd;

  delay(2000);
}
