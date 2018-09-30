// Original File

String InputDataString = "";
String Admin_ID_String = "4F00BE58359C";
unsigned char i = 0;
unsigned long Time = 0;

#define Buzzer 36
#define LockPin 35
#define LimitSwitch 34
#define Serial Serial1
#define RSTPIN 33
#define SSPIN 31

#include <SPI.h>
#include <SD.h>
#include <OneWire.h>
#include <Wire.h>
#include "RTClib.h"
#include <avr/wdt.h>
#include <MFRC522.h>

MFRC522 rfid(SSPIN, RSTPIN); // Instance of the class

String SettingsData;


OneWire  ds(23);  // on pin 8 (a 4.7K resistor is necessary)
OneWire  ds1(24);

File myFile;

//char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

unsigned long PreviousMillis;

unsigned int SDCardWriteCount = 0;

RTC_DS3231 rtc;

String DeviceName = "00009";
String RecievingData[7];
unsigned char Temp= 0;
unsigned char EndOfLine = 0;
unsigned char HigherTempLimit = 10, LowerTempLimit = 2;
String AlarmON = "1", LockEnabled = "1";
unsigned char UpdateRate = 10, DeviceLevel = 0;
unsigned long TempTimer = 0;
unsigned long DelayTime = 0;

#define chipSelect  13  //Chip select for SD card
//  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;
  DateTime CurrentTime;

void ProcessIncomingData(String IncomingData){
  //Serial.print("Data to Fridge: ");
  //Serial.println(IncomingData);
  if(IncomingData.startsWith(DeviceName+",O;")){
    OpenDoor();
  }
  else if(IncomingData.startsWith(DeviceName+",X;")){
    DontOpenDoor();
  }
  else if(IncomingData.startsWith(DeviceName+",N;")){
    NotRegistered();
  }
  else if(IncomingData.startsWith(DeviceName+",H;")){
    digitalWrite(Buzzer,HIGH);
    delay(1000);
    digitalWrite(Buzzer,LOW);
  }
  else if(IncomingData.startsWith(DeviceName+",@;")){
    UpdateDataInSDCard();
  }
  else if(IncomingData.startsWith(DeviceName+",U")){    //00002,U,005,008,002;
    //Serial.println(IncomingData.substring(11,14));
    //UpdateRate = (IncomingData.substring(8,11).toInt());
    SaveSettings(IncomingData);
    String SplitString[5];
    for(i = 0; i < 4; i++){
      SplitString[i] = IncomingData.substring(0,IncomingData.indexOf(','));
      IncomingData = IncomingData.substring(IncomingData.indexOf(',')+1);
    }
    SplitString[4] = IncomingData.substring(0,3);
    UpdateRate = SplitString[2].toInt();
    HigherTempLimit = SplitString[3].toInt();
    LowerTempLimit = SplitString[4].toInt();
    LowerTempLimit = constrain(LowerTempLimit,2,8);
  }
  wdt_reset();
}

String PackageLogData(){
        String PrepareData;
        PrepareData = "!,";
        PrepareData += DeviceName;
        PrepareData += ","; 
        PrepareData += AverageTemp();
        PrepareData += ","; 
        PrepareData += PackageCurrentTime();
        PrepareData += ";";
        return PrepareData;
}

float CorrectedTemperature1(){
  float Temperature;
  do{
      Temperature = ReadTemperature();
//      wdt_reset();
    }while(Temperature < 0.0 || Temperature > 60.0);
  return Temperature;
}

float CorrectedTemperature2(){
  float Temperature;
  do{
      Temperature = ReadTemperature1();
//      wdt_reset();
    }while(Temperature < 0.0 || Temperature > 60.0);
  return Temperature;
}

String PackageAccessData(){
        String PrepareData;
        PrepareData = "#";
        PrepareData += DeviceName;
        PrepareData += ",";
        PrepareData += InputDataString;
        PrepareData += ",";
        InputDataString = "";
        PrepareData += PackageCurrentTime();
        PrepareData += ";";
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
        //Serial.print("Current Time: ");
        //Serial.println(CurrentTimeString);
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
    Serial.println("*,"+DeviceName+";");
    while((!Serial.available()) && ((millis() - CurrentMillisCount) < 1000));
    wdt_reset();
    if(Serial.available()){
      String TempSerialData = Serial.readString();
        if(TempSerialData.startsWith("^,"+DeviceName+";")){
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

void LoadSettings(){
  if(SD.exists("Settings.txt")){
    myFile = SD.open("Settings.txt");
    //delay(1000);
    if(myFile.available()){
          char TempCollector;
          TempCollector = char(myFile.read());
          while(TempCollector != ';'){
            SettingsData += char(TempCollector);
            TempCollector = char(myFile.read());
          }
          SettingsData += char(';');
          Serial.println(SettingsData);

          String SplitString[5];
          for(i = 0; i < 4; i++){
            SplitString[i] = SettingsData.substring(0,SettingsData.indexOf(','));
            SettingsData = SettingsData.substring(SettingsData.indexOf(',')+1);
          } 
          SplitString[4] = SettingsData.substring(0,3);
          UpdateRate = SplitString[2].toInt();
          HigherTempLimit = SplitString[3].toInt();
          LowerTempLimit = SplitString[4].toInt();  
          LowerTempLimit = constrain(LowerTempLimit,2,8);

          wdt_reset();
          delay(20);
        myFile.close();
        delay(1000);
        wdt_reset();
    }
    else{
      Serial.println(F("File not available"));
    }
  }
  else{
    Serial.println(F("Settings file not found."));
  }
  wdt_reset();  
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
          delay(1000);
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

void SaveSettings(String Settings){
//  00002,U,005,008,002;
  if(SD.exists("Settings.txt")){ 
    Serial.println(F("Settings File Exists"));  
    wdt_reset();
    SD.remove("Settings.txt");
  }
  delay(1000);
    while(!(myFile = SD.open("Settings.txt", FILE_WRITE)));                       // open/create a file sensor.txt
        myFile.print(Settings);
    myFile.close();
    Serial.println(F("Settings Saved!!"));
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

float AverageTemp(){    //Lets try this tomorrow!!
  return float((CorrectedTemperature1() + CorrectedTemperature2())/2.0F);
  //return CorrectedTemperature1();
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

float ReadTemperature1(){
  if ( !ds1.search(addr)) {
    ds1.reset_search();
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

  ds1.reset();
  ds1.select(addr);
  ds1.write(0x44, 1);        // start conversion, with parasite power on at the end
  delay(1000);              //DONT REMOVE THIS
  present = ds1.reset();
  ds1.select(addr);    
  ds1.write(0xBE);         // Read Scratchpad

  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds1.read();
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

void ReadRFID(){
  if (rfid.PICC_IsNewCardPresent()){
  digitalWrite(Buzzer,HIGH);
  delay(100);
  digitalWrite(Buzzer,LOW);
  wdt_reset();
  String CurrentSwipe = "";
  if (  rfid.PICC_ReadCardSerial()){
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  CurrentSwipe += rfid.uid.uidByte[0];
  CurrentSwipe += rfid.uid.uidByte[1];
  CurrentSwipe += rfid.uid.uidByte[2];
  CurrentSwipe += rfid.uid.uidByte[3];
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  InputDataString = CurrentSwipe;
  Serial.println(PackageAccessData());
  InputDataString = "";
}
}
}

void setup(){
    pinMode(Buzzer,OUTPUT);
    pinMode(LockPin,OUTPUT);
    pinMode(LimitSwitch,INPUT_PULLUP);
    digitalWrite(Buzzer,HIGH);
    delay(100);
    digitalWrite(Buzzer,LOW);
    Lock();

    Serial.begin(1200);
    delay(1000);  
    if (! rtc.begin()) {
      Serial.println("RCT Issue");
    }

    if (rtc.lostPower()) {
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    delay(1000);
    wdt_enable(WDTO_8S);
//    Serial.println("Initializing RFID");
//    pinMode(13,OUTPUT);
//    digitalWrite(13,HIGH);    //This works when you Disable SD card in TecPharma Board
//    SPI.begin(); // Init SPI bus
//    rfid.PCD_Init(); // Init MFRC522 
    
    Serial.print("Initializing SD card...");                                                      
   
    pinMode(10, OUTPUT);                                                          // must initialize the hardware SS pin as output eventhough it is not using.
    digitalWrite(31,HIGH);
    digitalWrite(13,LOW);
    delay(1000);
    while(!SD.begin(13));                                                 // initialize the SD card 
    delay(10);                                        // initialize the SD card 
    Serial.println(F("card initialized."));
    wdt_reset();
    LoadSettings();
    Serial.print(F("Update Rate: "));     Serial.println(UpdateRate);
    Serial.print(F("High Temp Limit: ")); Serial.println(HigherTempLimit);
    Serial.print(F("Low Temp Limit: ")); Serial.println(LowerTempLimit);
    Lock();
    wdt_reset();
}

void loop(){
Lock();
wdt_reset();
i=0;
//CurrentTime = rtc.now();
//while(!rfid.PICC_IsNewCardPresent()){
  Lock();
  wdt_reset();
 if(Serial.available())
 ProcessIncomingData(Serial.readString());
 CurrentTime = rtc.now();
// if(CurrentTime.second() % UpdateRate == 0){
  if(AverageTemp() < HigherTempLimit && AverageTemp() > LowerTempLimit)   DelayTime = UpdateRate;
  else  DelayTime = 1;
  if(millis() - PreviousMillis > (DelayTime * 1000 * 60)){
    PreviousMillis = millis();
//  if(CurrentTime.minute() % UpdateRate == 0 && CurrentTime.second() == 0){
   if(IsUSBConnected()){
     Serial.println(PackageLogData());
     UpdateDataInSDCard();  
    }
    else{
       LogDataToCard(PackageLogData());
       Serial.println(F("Writing to Card"));
    }
  }
//  CurrentTime = rtc.now();
//  if(CurrentTime.minute() % UpdateRate == 0 && CurrentTime.second() == 0){
//  //if(CurrentTime.second() % UpdateRate == 0){
//  if(IsUSBConnected()){
//    Serial.println(PackageLogData());
//    UpdateDataInSDCard();  
//  }
//  else{
//    LogDataToCard(PackageLogData());
//    Serial.println(F("Writing to Card"));
//  }
//}
//}
//  ReadRFID();
}


