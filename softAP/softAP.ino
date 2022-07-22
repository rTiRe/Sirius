#include <iostream>
#include <vector>

#include "WiFi.h"
#include "AsyncUDP.h"
#include "ESPmDNS.h"
#include "WiFiAP.h"

#include "Struct.h"

// Определяем количество плат ESP32 в сети
#define NBOARDS 256

// Определяем номер этой платы
unsigned int NUM = -1;

// Определяем имя платы в сети
const char* master_host = "esp32master";

// Массив структур для обмена
multidata data[NBOARDS] {0};

// Определяем название и пароль точки доступа
const char* SSID = "Anchor0";
const char* PASSWORD = "Sirius12345";
const uint16_t PORT = 49152;
int c = 0;
AsyncUDP udp;

void parsePacket(AsyncUDPPacket packet)
{
  if(NUM >= 0) {
    // Преобразуем указатель на данные к указателю на структуру
    const multidata* tmp = (multidata*)packet.data();
  
    // Вычисляем размер данных (ожидаем получить размер в один элемент структур)
    const size_t len = packet.length() / sizeof(data[0]);
  
    // Если указатель на данные не равен нулю и размер данных больше нуля...
    if (tmp != nullptr && len > 0) {
      data[tmp->num].num = tmp->num;
      data[tmp->num].boardIP = tmp->boardIP;
      data[tmp->num].mode = tmp->mode;
      data[tmp->num].sendTo = tmp->sendTo;
      data[tmp->num].message = tmp->message;
      data[tmp->num].RSSI1From = tmp->RSSI1From;
      data[tmp->num].RSSI1 = tmp->RSSI1;
      data[tmp->num].RSSI2From = tmp->RSSI2From;
      data[tmp->num].RSSI2 = tmp->RSSI2;
      data[tmp->num].RSSI3From = tmp->RSSI3From;
      data[tmp->num].RSSI3 = tmp->RSSI3;
      data[tmp->num].APsX[8] = tmp->APsX[8];
      data[tmp->num].APsY[8] = tmp->APsY[8];
  
      // Отправляем данные всех плат побайтово
      packet.write((uint8_t*)data, sizeof(data));
    }
  }
}

void setup()
{
  Serial.begin(115200);
  
  WiFi.softAP(SSID, PASSWORD);
  NUM = WiFi.softAPIP()[3] - 1;
  // --->  WiFi.setTxPower(WIFI_POWER_19_5dBm); // устанавливаем TxPower
  // --->  Serial.println(WiFi.getTxPower()); // получаем TxPower
  
  if(NUM >= 0) {
    data[NUM].mode = "master";
    data[NUM].boardIP = WiFi.softAPIP();
  
    if (!MDNS.begin(master_host)) {
      Serial.println(data[NUM].boardIP);
    }
    if (udp.listen(PORT)) {
      udp.onPacket(parsePacket);
    }
    Serial.println("Сервер запущен.");
  }
}

void loop()
{
  int result = mySensor.accelUpdate();
  if (result == 0)  {
    aSqrt = mySensor.accelSqrt();
    aSqrt = median(aSqrt);
    //Serial.println(aSqrt);
    sensorVal = digitalRead(12);

  gas = analogRead(PIN_MQ2);
  val = analogRead(analogMQ7);   // Считываем значение с порта A5
  

  if (val > 350 or gas > 2000 ){
    digitalWrite(13, HIGH);
      }
      
   else if (val < 350 or gas < 2000){
        digitalWrite(13, LOW);
      }

    
  
  // выводим значения газов в ppm
 
    updateAccState(aSqrt);
    counter = 0;
    if(((accState == 2)||(accState == 3)||(accState == 4)
    
        ) 
        &&(acc_max < aSqrt)){
        acc_max = aSqrt;
    }
  }    
  else  {
    Serial.println("Sorry, it is imposible");
  }

  
  if(NUM >= 0) {
    for (size_t i = 0; i < sizeof(data)/sizeof(*data); i++) {
      if(data[i].mode == "slave" && (data[i].sendTo == "master" || data[i].sendTo == String(WiFi.softAPIP())) && (data[i].message == "1" || data[i].message == "0")) {
        for(int k = 0; k < 8; k++) {
          if(data[i].message == "0") {
            if(data[NUM].APsX[k] != data[i].num || (data[NUM].APsX[k] == data[i].num && data[i].message == "0")) {
              if(data[NUM].APsX[k] == 0 || (data[NUM].APsX[k] == data[i].num && data[i].message == "0")) {
                data[0].APsX[k] = data[i].num;
                data[NUM].sendTo = String(data[i].boardIP[0]) + "." + String(data[i].boardIP[1]) + "." + String(data[i].boardIP[2]) + "." + String(data[i].boardIP[3]);
                data[NUM].message = "2";
                break;
              }
            } else {
              data[NUM].sendTo = "";
              data[NUM].message = "";
              break;
            }
          } else {
            if(data[NUM].APsY[k] != data[i].num || (data[NUM].APsY[k] == data[i].num && data[i].message == "1")) {
              if(data[NUM].APsY[k] == 0 || (data[NUM].APsY[k] == data[i].num && data[i].message == "1")) {
                data[0].APsY[k] = data[i].num;
                data[NUM].sendTo = String(data[i].boardIP[0]) + "." + String(data[i].boardIP[1]) + "." + String(data[i].boardIP[2]) + "." + String(data[i].boardIP[3]);
                data[NUM].message = "2";
                break;
              }
            } else {
              data[NUM].sendTo = "";
              data[NUM].message = "";
              break;
            }
          }
        }
      } else {
        data[NUM].sendTo = "";
        data[NUM].message = "";
      }
      Serial.print("Порядковый номер: ");
      Serial.print(data[i].num);
      Serial.print(", IP адрес платы: ");
      Serial.print(data[i].boardIP);
      Serial.print(", режим работы: ");
      Serial.print(data[i].mode);
      Serial.print(", sendTo: ");
      Serial.print(data[i].sendTo);
      Serial.print(", message: ");
      Serial.print(data[i].message);
      if(data[i].mode == "client") {
        Serial.print(",   RSSI: ");
        Serial.print(data[i].RSSI1);
        Serial.print(", ");
        Serial.print(data[i].RSSI2);
        Serial.print(", ");
        Serial.print(data[i].RSSI3);
      }
      Serial.print(",   APs: ");
      Serial.print(data[NUM].APsX[0]);
      Serial.print(data[NUM].APsY[0]);
      Serial.println(";");
    }
    Serial.println();
    Serial.println("----------------------------");
    Serial.println();
  }
  delay(1000);
}
