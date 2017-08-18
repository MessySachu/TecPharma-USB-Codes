#define RFID Serial2
#define Radio Serial1
#define PowerLED 7
#define DataLED 6

int buzzer = 3;
void setup() 
{
  // put your setup code here, to run once:
  pinMode(2,OUTPUT);
  pinMode(2,LOW);
  pinMode(buzzer,OUTPUT);
  digitalWrite(buzzer,LOW);
  pinMode(PowerLED,OUTPUT);
  pinMode(DataLED,OUTPUT);
  analogWrite(PowerLED,1);
  //digitalWrite(PowerLED,HIGH);
  digitalWrite(DataLED,LOW);
  
  Serial.begin(1200);
  RFID.begin(9600);
  Radio.begin(1200);
}


void loop() { // run over and over
  if(RFID.available())
  {
    digitalWrite(buzzer,HIGH);
    delay(100);
    digitalWrite(buzzer,LOW);
    String TempData = "#,Swiper,";
    while(RFID.available()) {
      TempData += (char)RFID.read();
    }
    TempData += ";";
    Serial.println(TempData);
  }
 
  while(Radio.available()) {
    digitalWrite(DataLED,HIGH);
    Serial.write((char)Radio.read());
    digitalWrite(DataLED,LOW);
  }

  while(Serial.available()) {
    digitalWrite(DataLED,HIGH);
    Radio.write((char)Serial.read());
    digitalWrite(DataLED,LOW);
  }
}
