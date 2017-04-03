
#define DHT_PIN 2
#define IRDA_PIN 4
int _lastTime;

struct DHTResult {
  float Temperature;
  float Humidity;
  int Error;
  int Sum;
  int CheckSum;
};

void setup() {
  Serial.begin(9600);
  pinMode(DHT_PIN,INPUT);
  Serial.println("Loaded");
}

byte BitToByte(byte* start) {
  int result = 0;
  for (int i = 0; i < 8; i++) {
    result = result + start[i] * pow(2,7 - i);
  }
  return result;
}

byte dhtData[85];
int dataIterator;
  
DHTResult getTemperature() {
  dataIterator = 0;
  //спрашиваем датчик "как дела"
  pinMode(DHT_PIN,OUTPUT);
  digitalWrite(DHT_PIN,LOW);
  delay(25);
  digitalWrite(DHT_PIN,HIGH);
  //delayMicroseconds(30);
  pinMode(DHT_PIN,INPUT);

  //датчик отвечает "норм", сырые данные по задержкам:
  unsigned long lastAnswered = micros();
  int lastPinLevel = HIGH;
  int currentPinLevel = HIGH;
  unsigned long delta = 0;
  while (delta < 2000 && dataIterator < 90) {
    currentPinLevel = digitalRead(DHT_PIN);
    delta = micros() - lastAnswered;
    if (lastPinLevel != currentPinLevel && delta > 10) {
      dhtData[dataIterator] = delta;
      lastAnswered += delta;
      dataIterator++;
      lastPinLevel = currentPinLevel;
    }
  }

  //обрабатываем данные
  dataIterator = 0;
  for (int i = 0; i < 84; i++) {
    if (i >= 4 && i % 2 == 0) {
      int signalDelay = dhtData[i];
      if (signalDelay > 40) {
        dhtData[dataIterator] = 1;
      } else {
        dhtData[dataIterator] = 0;
      }
      dataIterator++;
    }
  }

  byte hum_int = BitToByte(&dhtData[0]);
  byte hum_dec = BitToByte(&dhtData[8]);
  byte temp_int = BitToByte(&dhtData[16]);
  byte temp_dec = BitToByte(&dhtData[24]);
  byte checksum = BitToByte(&dhtData[32]);
  int sum = hum_int + hum_dec + temp_int + temp_dec;
  DHTResult result;
  result.Temperature = temp_int + temp_dec / 256;
  result.Humidity = hum_int + hum_dec / 256;
  result.Sum = sum;
  result.CheckSum = checksum;
  return result;
}

byte irdaData[80];
byte irdaDataIterator = 0;

void onIrda() {
  irdaDataIterator++;
  unsigned long lastAnswered = micros();
  int lastPinLevel = HIGH;
  int currentPinLevel = HIGH;
  unsigned long delta = 0;
  while (delta < 10000 && irdaDataIterator < 69) {
    currentPinLevel = digitalRead(IRDA_PIN);
    delta = micros() - lastAnswered;
    if (lastPinLevel != currentPinLevel) {
      irdaData[irdaDataIterator] = delta;
      lastAnswered += delta;
      irdaDataIterator++;
      lastPinLevel = currentPinLevel;
    }
  }

  irdaDataIterator = 0;
  for (int i = 0; i < 69; i++) {
    if (i > 4 && i % 2 == 1) {
      byte signalDelay = irdaData[i];
      if (signalDelay > 127) {
        irdaData[irdaDataIterator] = 1;
      } else {
        irdaData[irdaDataIterator] = 0;
      }
      irdaDataIterator++;
    }
  }

  /*for (int i = 0; i < irdaDataIterator; i++) {
    Serial.print(i); Serial.print(": "); Serial.print(irdaData[i]);  Serial.println("; ");  
  }
  Serial.print("Get "); Serial.println(irdaDataIterator);*/
  byte a = BitToByte(&irdaData[0]);
  byte b = BitToByte(&irdaData[8]);
  byte c = BitToByte(&irdaData[16]);
  byte d = BitToByte(&irdaData[24]);

  irdaEvent(a, b, c, d);
  
  irdaDataIterator = 0;
}

unsigned long irdaPressedLastTime = micros(); 
bool isIrdaPressed;
byte memIrdaData1, memIrdaData2;
void irdaEvent(byte remoteControllerByte1, byte remoteControllerByte2, byte data1, byte data2) {
  if (remoteControllerByte1 == 0x3F && remoteControllerByte2 == 0xBD) {
    onIrdaPressed(data1, data2);
    memIrdaData1 = data1;
    memIrdaData2 = data2;
    irdaPressedLastTime = micros();
    isIrdaPressed = true;
  } else if (remoteControllerByte1 == 0 && remoteControllerByte2 == 3 && data1 == memIrdaData1 && data2 == memIrdaData2) {
    irdaPressedLastTime = micros();
  }
}

void onIrdaPressed(byte data1, byte data2) {
  Serial.println((String) "Pressed " + (int) data1 + " , " + (int) data2);
}

void onIrdaReleased(byte data1, byte data2) {
  Serial.println((String) "Released " + (int) data1 + (String) " , " + (int) data2);
}

unsigned long test = 0;

int lastResult = HIGH;
unsigned long timePassed;
unsigned long lastTime = millis();
unsigned long delta = 0;
unsigned int len = 0;

void receiveCommand(String cmd) {
  if (cmd.substring(0,4) == (String) "temp") {
      DHTResult result = getTemperature();
      Serial.print("{ data: [");
      Serial.print(result.Temperature);
      Serial.print(", ");
      Serial.print(result.Humidity);
      Serial.print(", ");
      Serial.print(result.Sum);
      Serial.print(", ");
      Serial.print(result.CheckSum);
      Serial.println("] }");
      return;
  }

  if (cmd.substring(0,4) == (String) "ping") {
      Serial.println("pong");
      return;
  }
  Serial.print("hm: ");
  Serial.print(cmd);
}

void loop() {
    byte irda = digitalRead(IRDA_PIN);
    if (irda == 0 && irdaDataIterator == 0) onIrda();
    delta = millis() - lastTime;
    
    if (Serial.available() && delta > 1000) {
        lastTime = lastTime + delta;
        String inputString = Serial.readString();
        if (inputString.length() > 0)
            receiveCommand(inputString);
    }

    if (isIrdaPressed && micros() - irdaPressedLastTime > 80000) {
      isIrdaPressed = false;
      onIrdaReleased(memIrdaData1,memIrdaData2);
      memIrdaData1 = 0;
      memIrdaData2 = 0;
    }
}
