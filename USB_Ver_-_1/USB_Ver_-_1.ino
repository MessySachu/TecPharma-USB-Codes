unsigned char InputData[15] = {'_','_','_','_','_','_','_','_','_','_','_','_','_','_','_',};
unsigned char i = 0;
unsigned long Time = 0;

#define Buzzer 11
//#include <SoftwareSerial.h>
#include <Ethernet.h>
#include <SPI.h>
#include <SD.h>
#include <utility/w5100.h>
#include<OneWire.h>
#include <Wire.h>
#include "RTClib.h"

OneWire  ds(8);  // on pin 8 (a 4.7K resistor is necessary)
#if defined(ARDUINO_ARCH_SAMD)
// for Zero, output on USB Serial console, remove line below if using programming port to program the Zero!
   #define Serial SerialUSB
#endif


File myFile;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//#define EEPROM_MIN_ADDR 10
//#define EEPROM_MAX_ADDR 500
#define LockPin 5
#define LimitSwitch 6


unsigned int SDCardWriteCount = 0;
RTC_DS1307 rtc;
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[] = { 192, 168, 0, 4 };
byte server[] = { 192, 168, 0, 3 }; // Google
char serverData[20];
EthernetClient client;

String DeviceName = "000001";
String RecievingData[7];
unsigned char Temp= 0;
unsigned char EndOfLine = 0;
unsigned char HigherTempLimit = 0, LowerTempLimit = 0, AlarmON = 0;
unsigned char UpdateRate = 0, DeviceLevel = 0;
unsigned long TempTimer = 0;
//SoftwareSerial mySerial(2, 3);

const int chipSelect = 4;   
//  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;
  DateTime CurrentTime;
  
void setup(){
    Serial.begin(57600);
    Serial1.begin(9600);
    delay(1000);  
    if (! rtc.begin()) {
    //Serial.println("Couldn't find RTC");
    }

    if (! rtc.isrunning()) {
      // Serial.println("RTC is NOT running!");
      // following line sets the RTC to the date & time this sketch was compiled
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      // This line sets the RTC with an explicit date & time, for example to set
      // January 21, 2014 at 3am you would call:
      // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    }

//    Serial.println("Requesting DHCP IP Address ... ");
//    Ethernet.begin(mac, ip);
//    W5100.setRetransmissionTime(0x07D0);
//    W5100.setRetransmissionCount(3);
//    Serial.print("Ethernet initialization succeeded. IP is ");
//    Serial.println(Ethernet.localIP());
//    Serial.println("connecting...");
//    if (client.connect(server, 8080)) {
//        Serial.println("connected to Server");
//    } else {
//        Serial.println("connection failed");
//    }
    delay(1000);
//    analogReference(INTERNAL);
    pinMode(Buzzer,OUTPUT);
    pinMode(LockPin,OUTPUT);
    pinMode(LimitSwitch,OUTPUT);
    digitalWrite(Buzzer,HIGH);
/*    if(EEPROM.read(10) == 255){
        write_StringEE(20, DeviceName);
        EEPROM.write(10,DeviceName.length()+1);
    }
    else{
        DeviceName = read_StringEE(20,EEPROM.read(10));
    }
    */
//    UpdateRate = EEPROM.read(11);
//    UpdateRate = 5;
//    DeviceLevel = EEPROM.read(12);
   
    Serial.print("Initializing SD card...");                                      
   
    pinMode(10, OUTPUT);                                                          // must initialize the hardware SS pin as output eventhough it is not using.
    while(!SD.begin(chipSelect));                                                 // initialize the SD card 
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
        AlarmON = MyDetails[5].toInt();
        DataLogFile.close();
    }
    else{
        File DataLogFile = SD.open("MyDetails.txt", FILE_WRITE);
        DataLogFile.print(DeviceName);
        DataLogFile.print(",1,10,10,3,0");
//        DeviceName = "No_Name";
        UpdateRate = 10;
        HigherTempLimit = 10;
        LowerTempLimit = 3;
        AlarmON = 0;
        DataLogFile.close();
      }
}

void loop(){
  i=0;
   CurrentTime = rtc.now();
   while(!Serial1.available()){
//     while(CurrentTime.second()%10 == 0){
//     if(client.available() && EndOfLine == 0) {
      if(Serial.available() && EndOfLine == 0){
//       Temp = getClientData();
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
       if(RecievingData[0].equals("@") && EndOfLine == 1){
         String UpdateDetails = "";
//           DeviceName = RecievingData[1];
           UpdateDetails += DeviceName;
           UpdateDetails += ",";
//           write_StringEE(20, DeviceName);
//           EEPROM.write(10,DeviceName.length()+1);
           UpdateRate = RecievingData[2].toInt();
           UpdateDetails += RecievingData[2];
           UpdateDetails += ",";
//           EEPROM.write(11,UpdateRate);
           DeviceLevel = RecievingData[3].toInt();
           UpdateDetails += RecievingData[3];
           UpdateDetails += ",";
//           EEPROM.write(12,DeviceLevel);
           HigherTempLimit = RecievingData[4].toInt();
           UpdateDetails += RecievingData[4];
           UpdateDetails += ",";
           LowerTempLimit = RecievingData[5].toInt();
           UpdateDetails += RecievingData[5];
           UpdateDetails += ",";
           AlarmON = RecievingData[6].toInt();
           UpdateDetails += RecievingData[6];
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
//        if(millis() - TempTimer >= UpdateRate * 1000){
//        DateTime now = rtc.now();
        CurrentTime = rtc.now();
        if(CurrentTime.second() % UpdateRate == 0){
        if(IsUSBConnected()){
          Serial.println(PackageLogData());
          UpdateDataInSDCard();  
      }
  //      if(client.connected()){
  //      client.println(PackageLogData());
  //      UpdateDataInSDCard();
  //      }
        else{
          LogDataToCard(PackageLogData());
        }
//      TempTimer = millis();
      }
}
//      if(millis() - TempTimer > UpdateRate * 1000){
//      DateTime now = rtc.now();
        CurrentTime = rtc.now();
        if(CurrentTime.second() % UpdateRate == 0){
        if(IsUSBConnected()){
//        Serial.println("USB COnnected");
          Serial.println(PackageLogData());
          UpdateDataInSDCard();
      }
        
//        if(client.connected()){
//        client.println(PackageLogData());
//        UpdateDataInSDCard();
//        }
        else{
          LogDataToCard(PackageLogData());
          
        }
      TempTimer = millis();
      }
      ReconnectClient();
}
   if(Serial1.available()){
       for(i = 0; i < 12; i++)
       {
         while(!Serial1.available());
         InputData[i] = Serial1.read();
       }
       if(IsUSBConnected()){
         Serial.println(PackageAccessData());
         Serial.println(PackageLogData());
       }
       else{
         ///////////////////////
         LogAccessDataToCard(PackageAccessData());
         LogDataToCard(PackageLogData());
       }
        
//  if(client.connected()){
//        client.println(PackageAccessData());
//       client.println(PackageLogData());
// }
}
  ReconnectClient();
}

String PackageAccessData(){
        String PrepareData;
        PrepareData = "#,";
//      PrepareData += read_StringEE(20,EEPROM.read(10));
        PrepareData += DeviceName;
        PrepareData += ",";
        for(i = 0; i < 12; i++)
          PrepareData += char(InputData[i]);
        PrepareData += ",";
        PrepareData += DeviceLevel;
        PrepareData += ",";
        PrepareData += UpdateRate; 
        PrepareData += ",";   
        PrepareData += PackageCurrentTime();
//        PrepareData += ";";   
        return PrepareData;
}

String PackageCurrentTime(){
        String CurrentTimeString;
        //DateTime now = rtc.now();
        CurrentTimeString += String(CurrentTime.year(),DEC);
        //Serial.print(now.year(),DEC);
        CurrentTimeString += ".";
        CurrentTimeString += String(CurrentTime.month(),DEC);
        CurrentTimeString += ".";
        CurrentTimeString += String(CurrentTime.day(),DEC);
        CurrentTimeString += ".";
        CurrentTimeString += String(CurrentTime.hour(),DEC);
        CurrentTimeString += ".";
        CurrentTimeString += String(CurrentTime.minute(),DEC);
        CurrentTimeString += ".";
        CurrentTimeString += String(CurrentTime.second(),DEC);
        return CurrentTimeString;
}

bool IsUSBConnected(){
    Serial.println("*");
    bool CableConnected = false;
    unsigned long CurrentMillisCount = millis();
    while((!Serial.available()) && millis() - CurrentMillisCount < 100);
    if(Serial.available()){
        if(Serial.readString() == "^"){
            return true;
        }
        else
          return false;
    }
    else
      return false;
}

void UpdateDataInSDCard(){
      if(SD.exists("DataLog1.txt")){
          Serial.println("Log data needs to be updated");
          //Serial.println(SDCardWriteCount);
          SDCardWriteCount = 0;
          File DataLogFile = SD.open("DataLog1.txt");
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
                  Serial.println(PrepareData1);
//                client.print(PrepareData1);
                  delay(1);
              }
              DataLogFile.close();
              SD.remove("DataLog1.txt");
              delay(1);
          //Serial.println("Deleted");
          }
     }
/*     if(SD.exists("AccessLog.txt")){
          Serial.println("Access data needs to be updated");
          //Serial.println(SDCardWriteCount);
          SDCardWriteCount = 0;
          File AccessLogFile = SD.open("AccessLog.txt");
          if(AccessLogFile){
              while(AccessLogFile.available()){
                String PrepareAccess;
                char TempCollector;
                TempCollector = char(AccessLogFile.read());
                while(TempCollector != ';'){
                  PrepareAccess += char(TempCollector);
                  TempCollector = char(AccessLogFile.read());
                }
                PrepareAccess += char(';');
                  Serial.println(PrepareAccess);
//                client.print(PrepareData1);
                  delay(1);
              }
              AccessLogFile.close();
              SD.remove("AccessLog.txt");
              delay(1);
          //Serial.println("Deleted");
          }
     }*/
}

void ReconnectClient(){
//  if(!client.connected()){ 
//    client.stop();
//  Serial.println("Requesting DHCP IP Address ... ");
//  Ethernet.begin(mac, ip);
//  W5100.setRetransmissionTime(0x07D0);
//  W5100.setRetransmissionCount(3);
//    Serial.print("Ethernet initialization succeeded. IP is ");
//    Serial.println(Ethernet.localIP());
//    Serial.println("Connecting...");
//    if (client.connect(server, 8080)) {
//      Serial.println("Connected to Server");
//    } else {
//      Serial.println("Connetion Failed");
//  }
//  } 
}

void LogDataToCard(String PrintToFile){
    Serial.println("Connection failed, Writing Data to Card");
    while(!(myFile = SD.open("DataLog1.txt", FILE_WRITE)));                       // open/create a file sensor.txt
        myFile.print(PrintToFile);
//        Serial.println(PrintToFile.length());
        SDCardWriteCount++;
        delay(10);
    myFile.close();
}

void LogAccessDataToCard(String PrintToFile){
    Serial.println("Connection failed, Writing Data to Card");
    while(!(myFile = SD.open("AccessLog.txt", FILE_WRITE)));                       // open/create a file sensor.txt
        myFile.print(PrintToFile);
//        Serial.println(PrintToFile.length());
        SDCardWriteCount++;
        delay(10);
    myFile.close();
}

String PackageLogData(){
        String PrepareData;
        PrepareData = "!,";
        PrepareData += DeviceName;
        PrepareData += ",";
        PrepareData += ReadTemperature();
        PrepareData += ",";
        for(i = 0; i < 12; i++)
        PrepareData +=  char(InputData[i]);
          i=0;  
        PrepareData += ",";
        if(digitalRead(2))  PrepareData += "1";
        else                PrepareData += "0";
        PrepareData += ",";
        PrepareData += DeviceLevel;
        PrepareData += ",";   
        PrepareData += PackageCurrentTime();
//        PrepareData += ";";
        return PrepareData;
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
//      Serial.println("  Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
//      Serial.println("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
//      Serial.println("  Chip = DS1822");
      type_s = 0;
      break;
    default:
//      Serial.println("Device is not a DS18x20 family device.");
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
//  Serial.print("  Temperature = ");
//  Serial.print(celsius);
//  Serial.println(" Celsius, ");
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
/*
bool write_StringEE(int Addr, String input)
{
    char cbuff[input.length()+1];
    input.toCharArray(cbuff,input.length()+1);
    return eeprom_write_string(Addr,cbuff);
}

String read_StringEE(int Addr,int length)
{
	String stemp="";
	char cbuff[length];
	eeprom_read_string(Addr,cbuff,length);
	for(int i=0;i<length-1;i++)
	{
		stemp.concat(cbuff[i]);//combines characters into a String
		delay(1);
	}
	return stemp;
}

boolean eeprom_is_addr_ok(int addr) {
	return ((addr >= EEPROM_MIN_ADDR) && (addr <= EEPROM_MAX_ADDR));
}

boolean eeprom_write_bytes(int startAddr, const byte* array, int numBytes) {
	int i;
	if (!eeprom_is_addr_ok(startAddr) || !eeprom_is_addr_ok(startAddr + numBytes)) {
		return false;
	}
	for (i = 0; i < numBytes; i++) {
		EEPROM.write(startAddr + i, array[i]);
	}
 	return true;
}
 
boolean eeprom_write_string(int addr, const char* string) {
	int numBytes; // actual number of bytes to be written
	numBytes = strlen(string) + 1;
	return eeprom_write_bytes(addr, (const byte*)string, numBytes);
}

boolean eeprom_read_string(int addr, char* buffer, int bufSize) {
	byte ch; // byte read from eeprom
	int bytesRead; // number of bytes read so far
	if (!eeprom_is_addr_ok(addr)) { // check start address
		return false;
	}
	if (bufSize == 0) { // how can we store bytes in an empty buffer ?
		return false;
	}
	if (bufSize == 1) {
		buffer[0] = 0;
		return true;
	}
	bytesRead = 0; // initialize byte counter
	ch = EEPROM.read(addr + bytesRead); // read next byte from eeprom
	buffer[bytesRead] = ch; // store it into the user buffer
	bytesRead++; // increment byte counter
	while ( (ch != 0x00) && (bytesRead < bufSize) && ((addr + bytesRead) <= EEPROM_MAX_ADDR) ) {
		// if no stop condition is met, read the next byte from eeprom
		ch = EEPROM.read(addr + bytesRead);
		buffer[bytesRead] = ch; // store it into the user buffer
		bytesRead++; // increment byte counter
	}
	if ((ch != 0x00) && (bytesRead >= 1)) {
		buffer[bytesRead - 1] = 0;
	}
	return true;
}

*/
char getClientData() {
      char readData = client.read();
    return readData;
}

