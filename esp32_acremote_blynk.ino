#include "secrets.h"
#define BLYNK_TEMPLATE_ID  SECRET_BLYNK_TEMPLATE_ID
#define BLYNK_TEMPLATE_NAME "AC Controller"
#define BLYNK_AUTH_TOKEN SECRET_BLYNK_AUTH_TOKEN

// WIFI CREDENTIALS
char ssid[] = SECRET_WIFI_SSID;    
char pass[] = SECRET_WIFI_PASS; 

#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

// WIRING 
const uint16_t kIrLedPin = 4;  // IR LED on GPIO 4
#define DHTPIN 27              // DHT Sensor on GPIO 27
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
IRsend irsend(kIrLedPin);
BlynkTimer timer;

// IR CODES 
const uint16_t kRawDataLen = 212; 
const uint16_t kFrequency = 38;

uint16_t rawDataOn[kRawDataLen] = {
  3798, 1978, 402, 550, 378, 1534, 382, 550, 
  402, 1526, 378, 534, 430, 1502, 378, 534, 
  434, 1498, 378, 550, 406, 1494, 378, 550, 
  434, 1498, 346, 1538, 378, 550, 402, 1510, 
  382, 546, 434, 1498, 342, 1542, 406, 1498, 
  402, 1502, 378, 534, 406, 550, 370, 1542, 
  406, 1494, 382, 546, 378, 562, 410, 546, 
  390, 550, 430, 1502, 378, 534, 410, 550, 
  402, 538, 434, 1498, 346, 1542, 378, 546, 
  410, 546, 378, 566, 406, 546, 410, 1510, 
  342, 1542, 406, 1494, 382, 546, 382, 558, 
  410, 546, 410, 1510, 382, 550, 370, 570, 
  406, 550, 370, 570, 398, 1514, 378, 550, 
  406, 550, 406, 1514, 370, 1514, 406, 1494, 
  378, 550, 406, 550, 370, 570, 406, 550, 
  346, 594, 406, 550, 370, 570, 406, 550, 
  342, 598, 406, 546, 346, 594, 406, 550, 
  406, 1498, 378, 550, 406, 550, 402, 538, 
  406, 550, 402, 538, 406, 550, 406, 534, 
  406, 550, 402, 538, 406, 550, 402, 538, 
  430, 1498, 378, 570, 378, 550, 346, 594, 
  406, 550, 346, 594, 406, 550, 346, 594, 
  406, 550, 346, 594, 406, 550, 406, 1514, 
  346, 566, 406, 550, 406, 1510, 346, 1538, 
  406, 1494, 406, 1494, 382, 550, 370, 570, 
  402, 550, 374, 566, 406, 550, 406, 1514, 
  338, 1546, 398, 1000
};

uint16_t rawDataOff[kRawDataLen] = {
  3746, 2030, 350, 662, 318, 1542, 322, 658, 
  322, 1554, 322, 618, 346, 1554, 326, 618, 
  346, 1554, 326, 630, 326, 1550, 322, 634, 
  350, 1554, 342, 1542, 326, 634, 370, 1514, 
  326, 606, 374, 1554, 346, 1542, 350, 1550, 
  350, 1554, 322, 594, 346, 610, 390, 1518, 
  374, 1526, 350, 582, 370, 570, 374, 582, 
  374, 566, 398, 1534, 346, 570, 370, 582, 
  374, 570, 398, 1530, 370, 1518, 346, 582, 
  374, 582, 370, 570, 374, 582, 374, 1546, 
  366, 1518, 374, 1526, 350, 582, 370, 570, 
  374, 582, 374, 566, 398, 1534, 346, 566, 
  374, 582, 374, 570, 398, 1530, 346, 602, 
  346, 582, 374, 1546, 346, 1546, 370, 1514, 
  346, 586, 370, 586, 370, 570, 370, 582, 
  374, 566, 374, 582, 374, 570, 370, 586, 
  370, 570, 370, 582, 374, 570, 370, 586, 
  370, 1546, 346, 582, 370, 574, 370, 586, 
  366, 570, 374, 582, 370, 570, 374, 582, 
  370, 574, 370, 586, 366, 574, 374, 582, 
  370, 1530, 346, 586, 370, 586, 374, 566, 
  374, 582, 374, 566, 374, 586, 370, 570, 
  374, 582, 370, 570, 374, 582, 394, 1518, 
  346, 582, 374, 582, 374, 1546, 370, 1518, 
  370, 1530, 370, 1534, 346, 566, 374, 582, 
  374, 566, 398, 1530, 370, 1518, 350, 1550, 
  350, 1550, 350, 1000
};

bool isAcOn = false;


void setup() {
  Serial.begin(115200);
  dht.begin();
  irsend.begin();
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  timer.setInterval(5000L, checkTemp);
}

void loop() {
  Blynk.run();
  timer.run();
}

// CHECK TEMPERATURE AND CONTROL AC
void checkTemp() {
  float temp = dht.readTemperature();
  if (isnan(temp)) return;
  Blynk.virtualWrite(V1, temp);   
  Blynk.virtualWrite(V2, isAcOn ? "ON" : "OFF");
  Serial.print("Temp: ");
  Serial.print(temp);
  Serial.println(isAcOn ? " [AC ON]" : " [AC OFF]");

  if (temp < 23 && isAcOn) {
    Serial.println("Turning AC Off...");
    irsend.sendRaw(rawDataOff, kRawDataLen, kFrequency);
    isAcOn = false;
  }
  else if (temp > 25 && !isAcOn) {
    Serial.println("Turning AC ON...");
    irsend.sendRaw(rawDataOn, kRawDataLen, kFrequency);
    isAcOn = true;
  }
}