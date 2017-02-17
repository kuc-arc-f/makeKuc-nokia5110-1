/*
 SoftwareSerial send, esp8266
 + SNTP Udp Client, DeepSleep Mode
 BETA ver: 0.9.2
 timezone= JST
 thanks :Arduino Time library
 http://playground.arduino.cc/code/time
*/
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Time.h>
#include <TimeLib.h>
#include <SoftwareSerial.h>
extern "C" {
#include <user_interface.h>
}

SoftwareSerial mySerial(4, 5); /* RX:D4, TX:D5 */
WiFiUDP udp;
const char* ssid = "";
const char* password = "";
//thingspeak, http Setting
const char* host = "api.thingspeak.com";
String mAPI_KEY="your-KEY";
//
unsigned int localPort = 8888;       // local port to listen for UDP packets
const char* timeServer = "ntp.nict.jp";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
uint32_t mTimerNTP=0;
uint32_t mTimerTmp=0;
uint32_t mTimerHTTP=0;
uint32_t mReceive_Start=0;
//const int mSleepSec= 600;
const int mSleepSec  = 300;
int mSTAT=0;
int mSTAT_Send =101;
int mSTAT_Recv=102;

//Struct
struct stParam{
 String dat1;
};
struct stParam mParam;

// send an NTP request to the time server at the given address
void sendNTPpacket(const char* address)
{
  Serial.print("sendNTPpacket : ");
  Serial.println(address);

  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0]  = 0b11100011;   // LI, Version, Mode
  packetBuffer[1]  = 0;     // Stratum, or type of clock
  packetBuffer[2]  = 6;     // Polling Interval
  packetBuffer[3]  = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

time_t readNTPpacket() {
  Serial.println("Receive NTP Response");
  udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
  unsigned long secsSince1900 = 0;
  // convert four bytes starting at location 40 to a long integer
  secsSince1900 |= (unsigned long)packetBuffer[40] << 24;
  secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
  secsSince1900 |= (unsigned long)packetBuffer[42] <<  8;
  secsSince1900 |= (unsigned long)packetBuffer[43] <<  0;
  return secsSince1900 - 2208988800UL;
}

time_t getNtpTime()
{
  while (udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);

  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      return readNTPpacket();
    }
   }

  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

//
void setup() {
  Serial.begin( 9600 );
  mySerial.begin( 9600 );
  Serial.println("#Start-setup-ssEsp");
  // wifi-start
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  WiFi.begin(ssid, password); 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  //NTP
  udp.begin(localPort);
  delay(2000);
Serial.println("#Call-getNtpTime");
  setSyncProvider(getNtpTime);
  delay(2000); 
Serial.println( String(year())+"-"+ String(month())+"-"+ String(day( ))+" "+String(hour())+":"+String(minute())+":"+String(second())  );       
  mTimerHTTP= millis()+ 5000;
  mSTAT= mSTAT_Send;
  Serial.println("#End-Setup");
}
//
void time_display(){
  time_t n = now();
  time_t t;
  char cD1[13+1];
  const char* fmtUTC    = "d1T%10d";
  // JST
  t= n+ (9 * SECS_PER_HOUR);
//  Serial.print("t=");
//  Serial.println( t);
  //Serial.print("JST : ");
  //Serial.println(s);
  sprintf(cD1, fmtUTC, t );
  mySerial.print( cD1 );
  //delay(500);
//Serial.print("cD1=");
//Serial.println(cD1 );
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
//
void set_Struct(String sHd ,String sDat){
      String sTmp= sDat.substring(2,10 );
      if(sHd=="d1"){
        float fNum= sTmp.toFloat();
        mParam.dat1     =  String( fNum );
      }
}
//
void proc_http(String sTemp ){
//Serial.print("connecting to ");
//Serial.println(host);  
      WiFiClient client;
      const int httpPort = 80;
      if (!client.connect(host, httpPort)) {
        Serial.println("connection failed");
        return;
      }
      String url = "/update?key="+ mAPI_KEY + "&field1="+ sTemp ;        
      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
        "Host: " + host + "\r\n" + 
        "Connection: close\r\n\r\n");
      delay(10);      
      int iSt=0;
      while(client.available()){
          String line = client.readStringUntil('\r');
//Serial.print(line);
      }    
}
String mBuff="";
// SoftwareSerial (UART) proc
void proc_UART(){
  while( mySerial.available() ){
     char c= mySerial.read();
     mBuff.concat(c );
//Serial.println( "#mBuff="+ mBuff );
    if(  mBuff.length() >= 10 ){      
        String sHead= mBuff.substring(0,2);
        boolean bChkHd= Is_validHead( sHead );
        if(bChkHd== true){
Serial.println( "#Hd="+ sHead );
Serial.println( "#mBuff.2="+ mBuff );
          set_Struct(sHead, mBuff );
          if (millis() > mTimerHTTP ){
            proc_http(mParam.dat1   ) ;
Serial.print("millis.SleepStart: ");
Serial.println(millis() );            
            ESP.deepSleep( mSleepSec * 1000 * 1000);            
          }
          mSTAT= mSTAT_Send;
Serial.print("millis.Loop: ");
Serial.println(millis() );          
//Serial.print("d1=");
//Serial.println( mParam.dat1 );            
//Serial.print("mBuff=");
//Serial.println( mBuff );            
        }
        mBuff="";
    }else{
      if(mReceive_Start > millis()+ 10000 ){
        mSTAT = mSTAT_Send;
      }      
    }
  }//end_while
}

//
void loop() {
  if (millis() > 30000 ){
Serial.print("millis.SleepStart.Over_30S: ");
Serial.println(millis() );           
       ESP.deepSleep( mSleepSec * 1000 * 1000);
  }
  if(mSTAT==mSTAT_Send ){
      if (millis() > mTimerTmp) {
         mTimerTmp = millis()+ 1000;
         time_display();
         mSTAT = mSTAT_Recv;
         mReceive_Start= millis();
      }  
  }else{
      proc_UART();
  }  
  
}



