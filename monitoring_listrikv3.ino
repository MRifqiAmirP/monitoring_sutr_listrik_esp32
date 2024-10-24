#include <PZEM004Tv30.h>
#include <SoftwareSerial.h>
#include <QueueArray.h> // You'll need to install a queue library like QueueArray

SoftwareSerial SIM800L(3, 2);  // RX, TX
SoftwareSerial hmi(11, 10);

PZEM004Tv30 pzem(&Serial1);
PZEM004Tv30 pzem2(&Serial2);
PZEM004Tv30 pzem3(&Serial3);

unsigned long currentTime;
unsigned long lastSmsTime = 0;
const unsigned long smsInterval = 15000;  // Interval 15 detik

bool sendingSms = false;
unsigned long smsStartTime;
String pesan;

QueueArray<String> smsQueue;

void setup() {
  Serial.begin(115200);
  SIM800L.begin(9600);
  hmi.begin(9600);

  while (!Serial) {}

  Serial.println("Arduino dengan Modul GSM siap");

  // Inisialisasi modul GSM
  delay(1000);
  Serial.println("SIM800L siap digunakan!");

  // Set format SMS ke teks
  SIM800L.write("AT+CMGF=1\r\n");
  delay(1000);
}

void loop() {
  currentTime = millis();

  float voltage = pzem.voltage();
  float current = pzem.current();
  float power = pzem.power();
  float energy = pzem.energy();
  float frequency = pzem.frequency();
  float pf = pzem.pf();

  float voltage2 = pzem2.voltage();
  float current2 = pzem2.current();
  float power2 = pzem2.power();
  float energy2 = pzem2.energy();
  float frequency2 = pzem2.frequency();
  float pf2 = pzem2.pf();

  float voltage3 = pzem3.voltage();
  float current3 = pzem3.current();
  float power3 = pzem3.power();
  float energy3 = pzem3.energy();
  float frequency3 = pzem3.frequency();
  float pf3 = pzem3.pf();

  // Check conditions and queue SMS messages
  checkAndQueueSms(current, current2, current3, voltage, voltage2, voltage3);

  // Process the SMS queue
  processSmsQueue();

  // Send data to HMI
  sendDataToHmi(current, current2, current3, voltage, voltage2, voltage3);
}

void checkAndQueueSms(float current, float current2, float current3, float voltage, float voltage2, float voltage3) {
  if (current > 0.10) {
    Serial.print("OVER CURRENT   ");
    Serial.print("current2: ");
    Serial.print(current2);
    Serial.print(" current3: ");
    Serial.println(current3);

    if (current2 > 0 && current3 > 0) {
      if (currentTime - lastSmsTime >= smsInterval) {
        pesan = "OVER CURRENT 0km, 15km, dan 30km";
        smsQueue.enqueue(pesan);
        lastSmsTime = currentTime;
      }
    } else if (current2 > 0) {
      if (currentTime - lastSmsTime >= smsInterval) {
        Serial.println("OVER CURRENT");
        pesan = "OVER CURRENT 0km dan 15km";
        smsQueue.enqueue(pesan);
        lastSmsTime = currentTime;
      }
    } else if (current3 > 0) {
      if (currentTime - lastSmsTime >= smsInterval) {
        pesan = "OVER CURRENT 0km dan 30km";
        smsQueue.enqueue(pesan);
        lastSmsTime = currentTime;
      }
    } else if (current2 == 0 && current3 == 0) {
      if (currentTime - lastSmsTime >= smsInterval) {
        pesan = "OVER CURRENT 0km";
        smsQueue.enqueue(pesan);
        lastSmsTime = currentTime;
      }
    }
  }

  if (voltage > 210) {
    Serial.println("OVER VOLTAGE");

    if (currentTime - lastSmsTime >= smsInterval) {
      pesan = "OVER VOLTAGE 0km";
      smsQueue.enqueue(pesan);
      lastSmsTime = currentTime;
    }
  } else if (voltage < 190) {
    Serial.println("OVER VOLTAGE");

    if (currentTime - lastSmsTime >= smsInterval) {
      pesan = "OVER VOLTAGE 0km";
      smsQueue.enqueue(pesan);
      lastSmsTime = currentTime;
    }
  }

  if (current2 > 1) {
    Serial.println("OVER CURRENT");

    if (currentTime - lastSmsTime >= smsInterval) {
      pesan = "OVER CURRENT 15km";
      smsQueue.enqueue(pesan);
      lastSmsTime = currentTime;
    }
  }

  if (voltage2 > 210) {
    Serial.println("OVER VOLTAGE");

    if (currentTime - lastSmsTime >= smsInterval) {
      pesan = "OVER VOLTAGE 15km";
      smsQueue.enqueue(pesan);
      lastSmsTime = currentTime;
    }
  } else if (voltage2 < 190) {
    Serial.println("OVER VOLTAGE");

    if (currentTime - lastSmsTime >= smsInterval) {
      pesan = "OVER VOLTAGE 15km";
      smsQueue.enqueue(pesan);
      lastSmsTime = currentTime;
    }
  }

  if (current3 > 1) {
    Serial.println("OVER CURRENT");

    if (currentTime - lastSmsTime >= smsInterval) {
      pesan = "OVER CURRENT 30km";
      smsQueue.enqueue(pesan);
      lastSmsTime = currentTime;
    }
  }

  if (voltage3 > 210) {
    Serial.println("OVER VOLTAGE");

    if (currentTime - lastSmsTime >= smsInterval) {
      pesan = "OVER VOLTAGE 30km";
      smsQueue.enqueue(pesan);
      lastSmsTime = currentTime;
    }
  } else if (voltage3 < 190) {
    Serial.println("OVER VOLTAGE");

    if (currentTime - lastSmsTime >= smsInterval) {
      pesan = "OVER VOLTAGE 30km";
      smsQueue.enqueue(pesan);
      lastSmsTime = currentTime;
    }
  }
}

void processSmsQueue() {
  if (!sendingSms && !smsQueue.isEmpty()) {
    pesan = smsQueue.dequeue();
    startSendSMS(pesan.c_str());
  }

  if (sendingSms) {
    if (smsStep == 1 && millis() - smsStartTime >= 1000) {
      SIM800L.print(pesan.c_str());
      smsStartTime = millis();
      smsStep = 2;
    } else if (smsStep == 2 && millis() - smsStartTime >= 1000) {
      SIM800L.write((char)26);
      smsStartTime = millis();
      smsStep = 3;
    } else if (smsStep == 3 && millis() - smsStartTime >= 5000) {
      while (SIM800L.available()) {
        SIM800L.read();
      }
      Serial.println("SMS Selesai Dikirim!");
      sendingSms = false;  // Pengiriman SMS selesai
    }
  }
}

void sendDataToHmi(float current, float current2, float current3, float voltage, float voltage2, float voltage3) {
  int decimalCurrent = (int)((current - (int)current) * 100);
  int decimalCurrent2 = (int)((current2 - (int)current2) * 100);
  int decimalCurrent3 = (int)((current3 - (int)current3) * 100);
  stringDecimal = (decimalCurrent < 10) ? "0" + (String)decimalCurrent : (String)decimalCurrent;
  stringDecimal2 = (decimalCurrent2 < 10) ? "0" + (String)decimalCurrent2 : (String)decimalCurrent2;
  stringDecimal3 = (decimalCurrent3 < 10) ? "0" + (String)decimalCurrent3 : (String)decimalCurrent3;

  int intCurrent = (int)current;
  hmiSendInt("i1", intCurrent);
  hmiSendString("i1Decimal", stringDecimal);

  int intCurrent2 = (int)current2;
  hmiSendInt("i2", intCurrent2);
  hmiSendString("i2Decimal", stringDecimal2);

  int intCurrent3 = (int)current3;
  hmiSendInt("i3", intCurrent3);
  hmiSendString("i3Decimal", stringDecimal3);

  int intVolt = (int)voltage;
  hmiSendInt("v1", intVolt);

  int intVolt2 = (int)voltage2;
  hmiSendInt("v2", intVolt2);

  int intVolt3 = (int)voltage3;
  hmiSendInt("v3", intVolt3);

  if (current > 0.10) {
    hmi.print("konslet.pic=");
    hmi.print(1);
  } else {
    hmi.print("konslet.pic=");
    hmi.print(2);
  }
  hmi.write(0xff);
  hmi.write(0xff);
  hmi.write(0xff);
}

void startSendSMS(const char* message) {
  SIM800L.write("AT+CMGS=\"085366830291\"\r\n");
  smsStartTime = millis();
  sendingSms = true;
  smsStep = 1;
}

void hmiSendInt(String id, int value) {
  hmi.print(id);
  hmi.print(".val=");
  hmi.print(value);
  hmi.write(0xff);
  hmi.write(0xff);
  hmi.write(0xff);
}

void hmiSendString(String id, String value) {
  hmi.print(id);
  hmi.print(".txt=\"");
  hmi.print(value);
  hmi.print("\"");
  hmi.write(0xff);
  hmi.write(0xff);
  hmi.write(0xff);
}
