unsigned char InputData[15] = {'_','_','_','_','_','_','_','_','_','_','_','_','_','_','_',};
unsigned char Admin_ID[12] = {'4','F','0','0','B','E','5','8','3','5','9','C'};
String InputDataString = "";
String Admin_ID_String = "4F00BE58359C";
unsigned char Admin_Name[12] = {'a','d','m','i','n',' ',' ',' ',' ',' ',' ',' '};
unsigned char i = 0;
unsigned long Time = 0;

#define RTC_Vcc A3
#define RTC_Gnd A2

#define Buzzer 10
#include <SPI.h>
#include <SD.h>
#include <OneWire.h>
#include <Wire.h>
#include "RTClib.h"
#include <SoftwareSerial.h>

SoftwareSerial mySerial(2, 3);

OneWire  ds(A0);  // on pin 8 (a 4.7K resistor is necessary)

//#if defined(ARDUINO_ARCH_SAMD)
// for Zero, output on USB Serial console, remove line below if using programming port to program the Zero!
//   #define Serial SerialUSB
//#endif


File myFile;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

#define LockPin 5
#define LimitSwitch 6

unsigned int SDCardWriteCount = 0;
RTC_DS1307 rtc;

String DeviceName = "00001";
String RecievingData[7];
unsigned char Temp= 0;
unsigned char EndOfLine = 0;
unsigned char HigherTempLimit = 0, LowerTempLimit = 0;
String AlarmON = "1", LockEnabled = "1";
unsigned char UpdateRate = 0, DeviceLevel = 0;
unsigned long TempTimer = 0;
//SoftwareSerial mySerial(2, 3);

const int chipSelect = 9;  //Chip select for SD card
//  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;
  DateTime CurrentTime;
  
void setup(){
    pinMode(RTC_Vcc,OUTPUT);
    pinMode(RTC_Gnd,OUTPUT);

    digitalWrite(RTC_Vcc,HIGH);
    digitalWrite(RTC_Gnd,LOW);

    mySerial.begin(9600);
    Serial.begin(57600);
    delay(1000);  
    if (! rtc.begin()) {
    }

    if (! rtc.isrunning()) {
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    delay(1000);
    pinMode(Buzzer,OUTPUT);
    NotRegistered();
    pinMode(LockPin,OUTPUT);
    pinMode(LimitSwitch,OUTPUT);
    digitalWrite(Buzzer,HIGH);
   
//    Serial2.println("Initializing SD card...");
    Serial.print("Initializing SD card...");                                                      
   
    pinMode(10, OUTPUT);                                                          // must initialize the hardware SS pin as output eventhough it is not using.
    while(!SD.begin(chipSelect));                                                 // initialize the SD card 
    delay(10);                                        // initialize the SD card 
    Serial.println("card initialized.");
    if(SD.exists("MyDetails.txt")){
        File DataLogFile = SD.open("MyDetails.txt");
        int i = 0;
        String MyDetails[6];
        char TempData;
        while(DataLogFile.available()){
            TempData = char(DataLogFile.read());
            if(TempData == ','){
                i++;
            }
            else{
                MyDetails[i]+=TempData;
            }
        }
//        DeviceName = MyDetails[0];
        DeviceLevel = MyDetails[1].toInt();
        UpdateRate = MyDetails[2].toInt();
        HigherTempLimit = MyDetails[3].toInt();
        LowerTempLimit = MyDetails[4].toInt();
        AlarmON = MyDetails[5];
        DataLogFile.close();
    }
    else{
        File DataLogFile = SD.open("MyDetails.txt", FILE_WRITE);
        DataLogFile.print(DeviceName);
        DataLogFile.print(",1,10,10,3,0");
//      DeviceName = "No_Name";
        UpdateRate = 10;
        HigherTempLimit = 10;
        LowerTempLimit = 3;
        AlarmON = "0";
        DataLogFile.close();
      }
      delay(1000);
      UpdateDataInSDCard();
}

void loop(){
  i=0;
   CurrentTime = rtc.now();
   while(!mySerial.available()){
      if(Serial.available() && EndOfLine == 0){
       Temp = Serial.read();
       if(Temp != ',' && Temp != ';'){
             RecievingData[i].concat(char(Temp));
         }
       else if(Temp == ';'){
             EndOfLine = 1;  
             i = 0;
         }
       else{ 
         i++;
       }
       if(RecievingData[0].equals("@")){
         Serial.print("Lol;");
       }
       if(RecievingData[0].equals("@") && EndOfLine == 1 && RecievingData[1].equals(DeviceName)){
         Serial.print("Device Data Updated;");
         String UpdateDetails = "";
           UpdateDetails += DeviceName;
           UpdateDetails += ",";
           DeviceLevel = RecievingData[2].toInt();
           UpdateDetails += RecievingData[2];
           UpdateDetails += ",";
           HigherTempLimit = RecievingData[3].toInt();
           UpdateDetails += RecievingData[3];
           UpdateDetails += ",";
           LowerTempLimit = RecievingData[4].toInt();
           UpdateDetails += RecievingData[4];
           UpdateDetails += ",";
           AlarmON = RecievingData[5];
           UpdateDetails += RecievingData[5];
           UpdateDetails += ",";
           LockEnabled = RecievingData[6];
           UpdateDetails += RecievingData[6];
           UpdateDetails += ",";
           UpdateRate = RecievingData[7].toInt();
           UpdateDetails += RecievingData[7];
           SD.remove("MyDetails.txt");
           File DetailsLogFile = SD.open("MyDetails.txt", FILE_WRITE);
           DetailsLogFile.print(UpdateDetails);
           DetailsLogFile.close();
           
           for(i = 0; i < 7; i++)
             RecievingData[i] = "";
           EndOfLine = 0;
           i=0;
       }
       else if(RecievingData[0].equals("O") && EndOfLine == 1){
           OpenDoor();
           for(i = 0; i < 5; i++)
             RecievingData[i] = "";    
           EndOfLine = 0;
           i=0;
       }
       else if(RecievingData[0].equals("X") && EndOfLine == 1){
           DontOpenDoor();
           for(i = 0; i < 5; i++)
             RecievingData[i] = "";
           EndOfLine = 0;
           i=0;
       }
       else if(RecievingData[0].equals("N") && EndOfLine == 1){
           NotRegistered();
           for(i = 0; i < 5; i++)
             RecievingData[i] = "";
           EndOfLine = 0;
           i=0;
       }
       else if(RecievingData[0].equals("&") && EndOfLine == 1){
           LogRFIDCard(RecievingData[1]);
           for(i = 0; i < 5; i++)
             RecievingData[i] = "";
           EndOfLine = 0;
           i=0;
       }
       
        CurrentTime = rtc.now();
        if(CurrentTime.minute() % UpdateRate == 0 && CurrentTime.second() == 0){
        if(IsUSBConnected()){
          Serial.println(PackageLogData());
          UpdateDataInSDCard();  
      }
        else{
          LogDataToCard(PackageLogData());
         Serial.println("Writing to Card");
        }
      }
}
        CurrentTime = rtc.now();
        if(CurrentTime.minute() % UpdateRate == 0 && CurrentTime.second() == 0){
        if(IsUSBConnected()){
          delay(1000);
//          Serial2.print(PackageLogData());
          Serial.println(PackageLogData());
          UpdateDataInSDCard();  
      }
        else{
          LogDataToCard(PackageLogData());
         Serial.println("Writing to Card");
        }
      }
}
   if(mySerial.available()){
         InputDataString += mySerial.readString();
         bool AdminLoggedIn = true;
         if(!InputDataString.equals(Admin_ID_String))
           AdminLoggedIn = false;
       if(AdminLoggedIn){
         InputDataString = "admin       ";
         OpenDoor();
       }
       else{
       String RFID_Tags;
          File DataLogFile = SD.open("RFID.txt.txt");
          if(DataLogFile){
              while(DataLogFile.available()){
                RFID_Tags += char(DataLogFile.read());
              }
           bool Registered_User = false;
           for( i = 0; i <= RFID_Tags.length() - InputDataString.length(); i++){
             if(RFID_Tags.substring(i,InputDataString.length() + i) == InputDataString){
               Registered_User = true;
             }
           }
         if(Registered_User){
           OpenDoor();
         }
         else{
           DontOpenDoor();
         }
       }}
       if(IsUSBConnected()){
         delay(1000);
//         Serial2.print(PackageAccessData());
         Serial.println(PackageAccessData());
       }
       else{
         Serial.println("Writing to Card");
         LogAccessDataToCard(PackageAccessData());
       }
      InputDataString = "";
}
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
//    Serial2.print("*,00001;");
    Serial.print("*,00001;");
    unsigned long CurrentMillisCount = millis();
//    while((!Serial2.available()) && ((millis() - CurrentMillisCount) < 10000));
    while((!Serial.available()) && ((millis() - CurrentMillisCount) < 10000));
//    if(Serial2.available()){
    if(Serial.available()){
      delay(5);
//      String TempSerialData = Serial2.readString();
      String TempSerialData = Serial.readString();
        if(TempSerialData.startsWith("^,00001;")){
            return true;
        }
        else{
          if(TempSerialData.startsWith("O;")){
            OpenDoor();
          }
          else if(TempSerialData.startsWith("X;")){
            DontOpenDoor();
          }
            return false;
          }
    }
    else{
      return false;
    }
}

void UpdateDataInSDCard(){
      if(SD.exists("DataLog1.txt")){
//          Serial2.println("Log data needs to be updated;");
          Serial.println("Log data needs to be updated;");
          SDCardWriteCount = 0;
          File DataLogFile = SD.open("DataLog1.txt");
          delay(1000);
          if(DataLogFile){
              while(DataLogFile.available()){
                String PrepareData1;
                char TempCollector;
                TempCollector = char(DataLogFile.read());
                while(TempCollector != ';'){
                  PrepareData1 += char(TempCollector);
                  TempCollector = char(DataLogFile.read());
                }
                PrepareData1 += char(';');
                if(IsUSBConnected()){
//                  Serial2.println(PrepareData1);
                  Serial.println(PrepareData1);
                  delay(2000);  
              }
              }
              DataLogFile.close();
              SD.remove("DataLog1.txt");
              delay(1);
          }
     }
}

void LogDataToCard(String PrintToFile){
    while(!(myFile = SD.open("DataLog1.txt", FILE_WRITE)));                       // open/create a file sensor.txt
        myFile.print(PrintToFile);
    myFile.close();
}

void LogAccessDataToCard(String PrintToFile){
    while(!(myFile = SD.open("DataLog1.txt", FILE_WRITE)));      
        myFile.print(PrintToFile);
    myFile.close();
}

void LogRFIDCard(String PrintToFile){
    while(!(myFile = SD.open("RFID.txt", FILE_WRITE)));      
        myFile.print(PrintToFile + ";");
    myFile.close();
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
  digitalWrite(Buzzer,LOW);
  delay(100);
  digitalWrite(Buzzer,HIGH);
  delay(100);
  digitalWrite(Buzzer,LOW);
  delay(100);
  digitalWrite(Buzzer,HIGH);
}

void Lock(){
  if(!digitalRead(LimitSwitch))
  digitalWrite(LockPin,HIGH);
}

void UnLock(){
  if(!digitalRead(LimitSwitch))
  digitalWrite(LockPin,LOW);
  digitalWrite(Buzzer,LOW);
  delay(100);
  digitalWrite(Buzzer,HIGH);
}

void DontOpenDoor(){
  digitalWrite(Buzzer,LOW);
  delay(100);
  digitalWrite(Buzzer,HIGH);
  delay(100);
  digitalWrite(Buzzer,LOW);
  delay(100);
  digitalWrite(Buzzer,HIGH);
  delay(100);
  digitalWrite(Buzzer,LOW);
  delay(100);
  digitalWrite(Buzzer,HIGH);
}

void NotRegistered(){
  digitalWrite(Buzzer,LOW);
  delay(1000);
  digitalWrite(Buzzer,HIGH);
}
