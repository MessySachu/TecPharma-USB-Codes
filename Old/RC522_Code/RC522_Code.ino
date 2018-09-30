#include <SPI.h>
#include <MFRC522.h>

constexpr uint8_t RST_PIN = 33;     // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = 31;     // Configurable, see typical pin layout above
 
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

void setup() { 
  pinMode(13,OUTPUT);
  digitalWrite(13,HIGH);    //This works when you Disable SD card in TecPharma Board
  Serial.begin(1200);
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 
}
 
void loop() {
  if (  rfid.PICC_IsNewCardPresent())
    Serial.println(ReadRFID());
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
