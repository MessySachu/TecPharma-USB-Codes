#include <SPI.h>
#include <MFRC522.h>

#define Radio Serial1
//#define PowerLED 7
#define DataLED 13

constexpr uint8_t RST_PIN = 33;     // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = 31;     // Configurable, see typical pin layout above
 
 
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

int Buzzer = 3;
void setup() {
  pinMode(13,OUTPUT);
  digitalWrite(13,HIGH);    //This works when you Disable SD card in TecPharma Board
  // put your setup code here, to run once:  
  Serial.begin(1200);
//  pinMode(2,OUTPUT);
//  pinMode(2,LOW);
  pinMode(Buzzer,OUTPUT);
  digitalWrite(Buzzer,LOW);
//  pinMode(PowerLED,OUTPUT);
  pinMode(DataLED,OUTPUT);
//  analogWrite(PowerLED,1);
//  //digitalWrite(PowerLED,HIGH);
 digitalWrite(DataLED,LOW);
  

    Serial.println("Initializing RFID");
    SPI.begin(); // Init SPI bus
    rfid.PCD_Init(); // Init MFRC522 

  Radio.begin(1200);
}

String ReadRFID(){
  String CurrentSwipe = "#,Swiper,";
  if (  rfid.PICC_ReadCardSerial()){
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  CurrentSwipe += rfid.uid.uidByte[0];
  CurrentSwipe += rfid.uid.uidByte[1];
  CurrentSwipe += rfid.uid.uidByte[2];
  CurrentSwipe += rfid.uid.uidByte[3];
  CurrentSwipe += ";";
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  return CurrentSwipe;
  }
}

void loop() { // run over and over
  if(rfid.PICC_IsNewCardPresent())
  {
    digitalWrite(Buzzer,HIGH);
    delay(100);
    digitalWrite(Buzzer,LOW);
    Serial.println(ReadRFID());
  }
 
  if(Radio.available()) {
    digitalWrite(DataLED,HIGH);
    Serial.write((char)Radio.read());
    digitalWrite(DataLED,LOW);
  }

  if(Serial.available()) {
    digitalWrite(DataLED,HIGH);
    Radio.write((char)Serial.read());
    digitalWrite(DataLED,LOW);
  }
}

