// Подключаем библиотеки
#include "WiFi.h"
#include "AsyncUDP.h"
#include "ESPmDNS.h"
#include "WiFiAP.h"

#include "Struct.h"

// Определяем количество плат ESP32 в сети
#define NBOARDS 4

// Определяем номер этой платы
unsigned int NUM = -1;

// Определяем имя платы в сети
const char* master_host = "esp32master";

// Массив структур для обмена
multidata data[NBOARDS] {0};

// Определяем название и пароль точки доступа
const char* SSID = "Anchor0";
const char* PASSWORD = "Sirius12345";

// Определяем порт
const uint16_t PORT = 49152;

int slaves = 0;
int c = 0;

// Создаём объект UDP соединения
AsyncUDP udp;




// Определяем callback функцию обработки пакета
void parsePacket(AsyncUDPPacket packet)
{
  if(NUM >= 0) {
    // Преобразуем указатель на данные к указателю на структуру
    const multidata* tmp = (multidata*)packet.data();
  
    // Вычисляем размер данных (ожидаем получить размер в один элемент структур)
    const size_t len = packet.length() / sizeof(data[0]);
  
    // Если указатель на данные не равен нулю и размер данных больше нуля...
    if (tmp != nullptr && len > 0) {
  
      // Записываем порядковый номер платы
      data[tmp->num].num = tmp->num;
      // Записываем IP адрес
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
  
      // Отправляем данные всех плат побайтово
      packet.write((uint8_t*)data, sizeof(data));
    }
  }
}





void setup()
{
  // Инициируем последовательный порт
  Serial.begin(115200);

  // Инициируем WiFi
  WiFi.softAP(SSID, PASSWORD);
  NUM = WiFi.softAPIP()[3] - 1;
  // --->  WiFi.setTxPower(WIFI_POWER_19_5dBm); // устанавливаем TxPower
  // --->  Serial.println(WiFi.getTxPower()); // получаем TxPower

  if(NUM >= 0) {
    // Записываем адрес текущей платы в элемент структуры
    data[NUM].boardIP = WiFi.softAPIP();
  
    if (!MDNS.begin(master_host)) {
      Serial.println(data[NUM].boardIP);
    }
  
    // Инициируем сервер
    if (udp.listen(PORT)) {
      // вызываем callback функцию при получении пакета
      udp.onPacket(parsePacket);
    }
    Serial.println("Сервер запущен.");
  }
}

void loop()
{
  if(NUM >= 0) {
    data[NUM].mode = "master";

    // Выводим значения элементов в последовательный порт
    for (size_t i = 0; i < sizeof(data)/sizeof(*data); i++) {
      if(data[i].mode == "slave" && (data[i].sendTo == "master" || data[i].sendTo == String(WiFi.softAPIP())) && data[i].message == "1") {
        for (size_t j = 0; j < sizeof(data)/sizeof(*data); j++) {
          if(data[j].mode == "slave") { slaves++; }
        }
        data[NUM].sendTo = String(data[i].boardIP[0]) + "." + String(data[i].boardIP[1]) + "." + String(data[i].boardIP[2]) + "." + String(data[i].boardIP[3]);
        data[NUM].message = String(slaves);
        slaves = 0;
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
      Serial.println(data[i].message);
      if(data[i].mode == "client") {
        Serial.print("RSS1From: ");
        Serial.print(data[i].RSSI1From);
        Serial.print(", RSSI1: ");
        Serial.println(data[i].RSSI1);

        Serial.print("RSS2From: ");
        Serial.print(data[i].RSSI2From);
        Serial.print(", RSSI2: ");
        Serial.println(data[i].RSSI2);

        Serial.print("RSS3From: ");
        Serial.print(data[i].RSSI3From);
        Serial.print(", RSSI1: ");
        Serial.println(data[i].RSSI3);
      }
      Serial.print(";\n");
    }
    Serial.println("----------------------------");
  }
  if(c == 15) {
    c = 0;
    data[NUM].sendTo = "slave";
    data[NUM].message = "3";
    
  }
  delay(1000);
}
