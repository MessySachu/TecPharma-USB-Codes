// Original File

String InputDataString = "";
String Admin_ID_String = "4F00BE58359C";
unsigned char i = 0;
unsigned long Time = 0;

#define RTC_Vcc A3
#define RTC_Gnd A2

#define Buzzer 5
#define LockPin 4
#define LimitSwitch 3

#include <SPI.h>
#include <SD.h>
#include <OneWire.h>
#include <Wire.h>
#include "RTClib.h"
#include <SoftwareSerial.h>
#include <avr/wdt.h>


SoftwareSerial mySerial(2, 6);

OneWire  ds(A0);  // on pin 8 (a 4.7K resistor is necessary)

File myFile;

//char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};


unsigned int SDCardWriteCount = 0;
RTC_DS1307 rtc;

String DeviceName = "00001";
String RecievingData[7];
unsigned char Temp= 0;
unsigned char EndOfLine = 0;
unsigned char HigherTempLimit = 0, LowerTempLimit = 0;
String AlarmON = "1", LockEnabled = "1";
unsigned char UpdateRate = 10, DeviceLevel = 0;
unsigned long TempTimer = 0;

const int chipSelect = 9;  //Chip select for SD card
//  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;
  DateTime CurrentTime;
  
void setup(){
    pinMode(Buzzer,OUTPUT);
    pinMode(LockPin,OUTPUT);
    pinMode(LimitSwitch,INPUT_PULLUP);
    digitalWrite(Buzzer,LOW);
    Lock();
    pinMode(RTC_Vcc,OUTPUT);
    pinMode(RTC_Gnd,OUTPUT);

    digitalWrite(RTC_Vcc,HIGH);
    digitalWrite(RTC_Gnd,LOW);

    mySerial.begin(9600);
    Serial.begin(1200);
    delay(1000);  
    if (! rtc.begin()) {
    }

    if (! rtc.isrunning()) {
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    delay(1000);
    wdt_enable(WDTO_8S);
    Serial.print("Initializing SD card...");                                                      
   
    pinMode(10, OUTPUT);                                                          // must initialize the hardware SS pin as output eventhough it is not using.
    while(!SD.begin(chipSelect));                                                 // initialize the SD card 
    delay(10);                                        // initialize the SD card 
    Serial.println(F("card initialized."));
    Lock();
    wdt_reset();
}

void loop(){
Lock();
wdt_reset();
i=0;
CurrentTime = rtc.now();
while(!mySerial.available()){
  Lock();
  wdt_reset();
 if(Serial.available())
 ProcessIncomingData(Serial.readString());
 CurrentTime = rtc.now();
 //if(CurrentTime.second() % UpdateRate == 0){
  if(CurrentTime.minute() % UpdateRate == 0 && CurrentTime.second() == 0){
   if(IsUSBConnected()){
     Serial.println(PackageLogData());
     UpdateDataInSDCard();  
    }
    else{
       LogDataToCard(PackageLogData());
       Serial.println(F("Writing to Card"));
    }
  }
  CurrentTime = rtc.now();
  if(CurrentTime.minute() % UpdateRate == 0 && CurrentTime.second() == 0){
  //if(CurrentTime.second() % UpdateRate == 0){
  if(IsUSBConnected()){
    Serial.println(PackageLogData());
    UpdateDataInSDCard();  
  }
  else{
    LogDataToCard(PackageLogData());
    Serial.println(F("Writing to Card"));
  }
}
}

if(mySerial.available()){
  digitalWrite(Buzzer,HIGH);
  delay(100);
  digitalWrite(Buzzer,LOW);
  wdt_reset();
  InputDataString = mySerial.readString();
  bool AdminLoggedIn = true;
  if(!InputDataString.equals(Admin_ID_String))
    AdminLoggedIn = false;
  if(AdminLoggedIn){
    InputDataString = "admin       ";
    OpenDoor();
  }
//  else{
//    String RFID_Tags;
//    myFile = SD.open("RFID.txt.txt");
//    if(myFile){
//      while(myFile.available()){
//      RFID_Tags += char(myFile.read());
//    }
//    bool Registered_User = false;
//    for( i = 0; i <= RFID_Tags.length() - InputDataString.length(); i++){
//      if(RFID_Tags.substring(i,InputDataString.length() + i) == InputDataString){
//        Registered_User = true;
//      }
//    }
//    if(Registered_User){
//      OpenDoor();
//    }
//    else{
//      DontOpenDoor();
//    }
//   }
// myFile.close();
// }
 if(IsUSBConnected()){
   delay(500);
   Serial.println(PackageAccessData());
 }
 else{
   Serial.println(F("Writing to Card"));
   LogAccessDataToCard(PackageAccessData());
 }
 InputDataString = "";
}
}

void ProcessIncomingData(String IncomingData){
  //Serial.print("Data to Fridge: ");
  //Serial.println(IncomingData);
  if(IncomingData.startsWith("00001,O;")){
    OpenDoor();
  }
  else if(IncomingData.startsWith("00001,X;")){
    DontOpenDoor();
  }
  else if(IncomingData.startsWith("00001,N;")){
    NotRegistered();
  }
  else if(IncomingData.startsWith("00001,H;")){
    digitalWrite(Buzzer,HIGH);
    delay(1000);
    digitalWrite(Buzzer,LOW);
  }
  wdt_reset();
}

String PackageLogData(){
        String PrepareData;
        PrepareData = "!,";
        PrepareData += DeviceName;
        PrepareData += ",";
        PrepareData += ReadTemperature();
        PrepareData += ",";
        PrepareData += PackageCurrentTime();
        PrepareData += ";";
        return PrepareData;
}

String PackageAccessData(){
        String PrepareData;
        PrepareData = "#";
        PrepareData += DeviceName;
        PrepareData += InputDataString;
        PrepareData += PackageCurrentTime_Access();
        return PrepareData;
}

String PackageCurrentTime(){
        String CurrentTimeString;
        CurrentTimeString += String(CurrentTime.year(),DEC);
        if(CurrentTime.month() < 10)  CurrentTimeString += "0";
        CurrentTimeString += String(CurrentTime.month(),DEC);
        if(CurrentTime.day() < 10)  CurrentTimeString += "0";
        CurrentTimeString += String(CurrentTime.day(),DEC);
        if(CurrentTime.hour() < 10)  CurrentTimeString += "0";
        CurrentTimeString += String(CurrentTime.hour(),DEC);
        if(CurrentTime.minute() < 10)  CurrentTimeString += "0";
        CurrentTimeString += String(CurrentTime.minute(),DEC);
        if(CurrentTime.second() < 10)  CurrentTimeString += "0";
        CurrentTimeString += String(CurrentTime.second(),DEC);
        return CurrentTimeString;
}

String PackageCurrentTime_Access(){
        String CurrentTimeString;
        int CurrentYear = CurrentTime.year();
        CurrentYear = CurrentYear % 100;
        CurrentTimeString += String(DeviceLevel,DEC);
        CurrentTimeString += String(CurrentYear,DEC);
        if(CurrentTime.month() < 10)  CurrentTimeString += "0";
        CurrentTimeString += String(CurrentTime.month(),DEC);
        if(CurrentTime.day() < 10)  CurrentTimeString += "0";
        CurrentTimeString += String(CurrentTime.day(),DEC);
        if(CurrentTime.hour() < 10)  CurrentTimeString += "0";
        CurrentTimeString += String(CurrentTime.hour(),DEC);
        if(CurrentTime.minute() < 10)  CurrentTimeString += "0";
        CurrentTimeString += String(CurrentTime.minute(),DEC);
        if(CurrentTime.second() < 10)  CurrentTimeString += "0";
        CurrentTimeString += String(CurrentTime.second(),DEC);
        CurrentTimeString += ";";
        return CurrentTimeString;
}

bool IsUSBConnected(){
    unsigned long CurrentMillisCount = millis();
    Serial.println(F("*,00001;"));
    while((!Serial.available()) && ((millis() - CurrentMillisCount) < 1000));
    wdt_reset();
    if(Serial.available()){
      String TempSerialData = Serial.readString();
        if(TempSerialData.startsWith("^,00001;")){
            return true;
        }
        else{
            ProcessIncomingData(TempSerialData);
            return false;
          }
    }
    else{
      return false;
    }
}

void UpdateDataInSDCard(){
if(SD.exists("DataLog1.txt")){
    Serial.println(F("Log data needs to be updated;"));
    SDCardWriteCount = 0;
    myFile = SD.open("DataLog1.txt");
    delay(1000);
    if(myFile.available()){
        while(myFile.available()){
          String PrepareData1;
          char TempCollector;
          TempCollector = char(myFile.read());
          while(TempCollector != ';'){
            PrepareData1 += char(TempCollector);
            TempCollector = char(myFile.read());
          }
          PrepareData1 += char(';');
//        if(IsUSBConnected())
          Serial.println(PrepareData1);
          wdt_reset();
          delay(20);
        }
        myFile.close();
        SD.remove("DataLog1.txt");
        delay(1);
    }
    else{
      Serial.println(F("File Unavailable"));
      }
}
  wdt_reset();
}

void LogDataToCard(String PrintToFile){
    while(!(myFile = SD.open("DataLog1.txt", FILE_WRITE)));                       // open/create a file sensor.txt
    //myFile = SD.open("DataLog1.txt", FILE_WRITE);
        myFile.print(PrintToFile);
    myFile.close();
  wdt_reset();
}

void LogAccessDataToCard(String PrintToFile){
    while(!(myFile = SD.open("DataLog1.txt", FILE_WRITE)));
    //myFile = SD.open("DataLog1.txt", FILE_WRITE);      
        myFile.print(PrintToFile);
    myFile.close();
  wdt_reset();
}

void LogRFIDCard(String PrintToFile){
    while(!(myFile = SD.open("RFID.txt", FILE_WRITE)));      
    //myFile = SD.open("RFID.txt", FILE_WRITE);
        myFile.print(PrintToFile + ";");
    myFile.close();
  wdt_reset();
}

float ReadTemperature(){
  if ( !ds.search(addr)) {
    ds.reset_search();
    delay(50);
//    return;
  }
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      type_s = 1;
      break;
    case 0x28:
      type_s = 0;
      break;
    case 0x22:
      type_s = 0;
      break;
    default:
      break;
  } 

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  delay(1000);              //DONT REMOVE THIS
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  return celsius;
}

void OpenDoor(){
  UnLock();
  wdt_reset();
  digitalWrite(Buzzer,HIGH);
  delay(100);
  digitalWrite(Buzzer,LOW);
  delay(100);
  digitalWrite(Buzzer,HIGH);
  delay(100);
  digitalWrite(Buzzer,LOW);
  wdt_reset();
  delay(4000);
  Lock();
  wdt_reset();
}

void Lock(){
  if(digitalRead(LimitSwitch))
    digitalWrite(LockPin,HIGH);
  else
    digitalWrite(LockPin,LOW);
}

void UnLock(){
  //if(!digitalRead(LimitSwitch))
  digitalWrite(LockPin,LOW);
  digitalWrite(Buzzer,HIGH);
  delay(100);
  digitalWrite(Buzzer,LOW);
}

void DontOpenDoor(){
  digitalWrite(Buzzer,HIGH);
  delay(100);
  digitalWrite(Buzzer,LOW);
  delay(100);
  digitalWrite(Buzzer,HIGH);
  delay(100);
  digitalWrite(Buzzer,LOW);
  delay(100);
  digitalWrite(Buzzer,HIGH);
  delay(100);
  digitalWrite(Buzzer,LOW);
}

void NotRegistered(){
  digitalWrite(Buzzer,HIGH);
  delay(1000);
  digitalWrite(Buzzer,LOW);
}
