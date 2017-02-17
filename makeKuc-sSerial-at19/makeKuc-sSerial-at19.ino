/*
 NOKIA 5110 LCD+ analog(LM60BIZ)
 NTP clock, sensor value display
 with mkuBord-44
*/
#include <SoftwareSerial.h>
#include <Time.h>
#include <TimeLib.h> 
#include "font.h"

SoftwareSerial mySerial(5, 6); /* RX:D5, TX:D6 */
#define PIN_RESET  2
#define PIN_SCE    3
#define PIN_DC    10
#define PIN_SDIN  11
#define PIN_SCLK  13

#define LCD_C     LOW
#define LCD_D     HIGH
#define LCD_X     84
#define LCD_Y     48
//
int mTemp=0;
//uint32_t mTimerTmp;
//uint32_t mTimerTime;
uint32_t mReceive_Start=0;
const int mVoutPin = 0;

void LcdCharacter(char character)
{
  LcdWrite(LCD_D, 0x00);
  for (int index = 0; index < 5; index++)
  {
    LcdWrite(LCD_D, ASCII[character - 0x20][index]);
  }
  LcdWrite(LCD_D, 0x00);
}

void LcdClear(void)
{
  for (int index = 0; index < LCD_X * LCD_Y / 8; index++)
  {
    LcdWrite(LCD_D, 0x00);
  }
}

void LcdInitialise(void)
{
  pinMode(PIN_SCE, OUTPUT);
  pinMode(PIN_RESET, OUTPUT);
  pinMode(PIN_DC, OUTPUT);
  pinMode(PIN_SDIN, OUTPUT);
  pinMode(PIN_SCLK, OUTPUT);
  digitalWrite(PIN_RESET, LOW);
  digitalWrite(PIN_RESET, HIGH);
  LcdWrite(LCD_C, 0x21 );  // LCD Extended Commands.
  LcdWrite(LCD_C, 0xB1 );  // Set LCD Vop (Contrast). 
  LcdWrite(LCD_C, 0x04 );  // Set Temp coefficent. //0x04
  LcdWrite(LCD_C, 0x14 );  // LCD bias mode 1:48. //0x13
  LcdWrite(LCD_C, 0x20 );  // LCD Basic Commands
  LcdWrite(LCD_C, 0x0C );  // LCD in normal mode.
}

void LcdString(char *characters)
{
  while (*characters)
  {
    LcdCharacter(*characters++);
  }
}

void LcdWrite(byte dc, byte data)
{
  digitalWrite(PIN_DC, dc);
  digitalWrite(PIN_SCE, LOW);
  shiftOut(PIN_SDIN, PIN_SCLK, MSBFIRST, data);
  digitalWrite(PIN_SCE, HIGH);
}
//
void setup(void)
{
  Serial.begin(9600);
  mySerial.begin( 9600 );
  Serial.println("# Start-setup");
  LcdInitialise();
  pinMode(mVoutPin, INPUT);
  LcdClear();
}
//
long convert_Map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
// reading LM60BIZ
int getTempNum(){
  int iRet=0;
  float fSen  = 0;
  unsigned long reading  = 0;
  int iDiv =3;  
  for (int i=0; i< iDiv; i++) {
    int  iTmp = analogRead(mVoutPin);
    reading  += iTmp; 
    delay(50);
  }
  int SValue= reading / iDiv;
  int voltage=convert_Map(SValue, 0, 1000, 0,3300);  // V
//Serial.print("SValue=");
//Serial.print(SValue);
//Serial.print(" , voltage=");
//Serial.println(voltage);
  int iTemp = (voltage - 424) / 6.25; // offset=425
  iRet= iTemp;
  
  return iRet;  
}
uint32_t mTimer=0;
// int mTemp=0;
//
void draw_string(String sTime, String sTemp)
{
  char *cTmp = sTemp.c_str();
  char *cTime= sTime.c_str();
  LcdClear();
  LcdString("TIME:       ");
//  LcdString(" 10:23:56   ");
  LcdString( cTime );
  LcdString("------------");
  LcdString("Temperature:");
  LcdString( cTmp );
  LcdString("            ");
}
//
String getString_line(String src)
{
  String sRet="";
  int iMax=12;
  String sEnd="            ";
  String sBuff=src + sEnd;
        sRet= sBuff.substring(0, iMax);
  return sRet;
}
//
boolean Is_validHead(String sHd){
  boolean ret=false;
  if(sHd=="d1"){
    ret= true;
  }else if(sHd=="d2"){
    ret= true;
  }
  return ret;
}
String  digitalClockDisplay(){
  String sRet="";
  // digital clock display of the time
  char cD1[8+1];
  time_t t = now();
  const char* fmtSerial = "%02d:%02d:%02d";
  sprintf(cD1, fmtSerial, hour(t), minute(t), second(t));
//Serial.println(cD1);
  sRet=String(cD1);
  return sRet;  
}
//
void conv_timeSync(String src){
  //String sRet="";
  unsigned long pctime;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013 
   pctime = src.toInt();
   if( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
     setTime(pctime); // Sync Arduino clock to the time received on the serial port
   }  
   //return sRet;
}
String mBuff="";
//
void proc_uart(){
    while( mySerial.available() ){
      char c= mySerial.read();
      mBuff.concat(c );
      if(  mBuff.length() >= 13 ){
        String sHead= mBuff.substring(0,2);
        boolean bChkHd= Is_validHead( sHead );
        if(bChkHd== true){
// Serial.println( "Hd="+ sHead );
Serial.println( "mBuff="+ mBuff );
          String sTmp= mBuff.substring(3,13);
          conv_timeSync(sTmp);
           //send
           int iD1=int(mTemp );
           char cD1[10+1];
           sprintf(cD1 , "d1%08d", iD1);       
           mySerial.print( cD1 );
           mReceive_Start=millis();
//Serial.print("cD1=");
//Serial.println(cD1  );                    
        }        
        mBuff="";
      }else{
          if(mReceive_Start > millis()+ 10000 ){
            mBuff="";
          }
      }
    } //end_while
}
//
void loop(void)
{
  proc_uart();
  if(millis() >mTimer ){
      mTimer=millis() + 3000;
      mTemp = getTempNum();
      String sTime= digitalClockDisplay();
      char sBuff[12+1];
      sprintf(sBuff, " %dC", mTemp ); 
      String sTmp     = getString_line( String(sBuff ) );
      String sTimeBuff= getString_line(  " "+ sTime );      
      draw_string(sTimeBuff ,  sTmp );
  }  
}



