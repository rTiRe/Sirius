// Подключаем библиотеки
#include "WiFi.h"
#include "AsyncUDP.h"
#include "ESPmDNS.h"

#include <cmath>

#include "Struct.h"

// Определяем количество плат
#define NBOARDS 4

// Определяем номер этой платы
unsigned int NUM = -1;

// Массив структур для обмена
multidata data[NBOARDS] {0};

/* Определяем имена для mDNS */
// для ведущей платы
const char* master_host = "esp32master";
// приставка имени ведомой платы
const char* slave_host = "esp32client";

// Определяем название и пароль точки доступа
const char* SSID = "Anchor0";
const char* PASSWORD = "Sirius12345";

// Определяем порт
const uint16_t PORT = 49152;

// Создаём объект UDP соединения
AsyncUDP udp;

float meters = 0;

int rs[3] = {0};

// Определяем callback функцию обработки пакета
void parsePacket(AsyncUDPPacket packet)
{
  if(NUM >= 0) {
    const multidata* tmp = (multidata*)packet.data();

    // Вычисляем размер данных
    const size_t len = packet.length() / sizeof(data[0]);

    // Если адрес данных не равен нулю и размер данных больше нуля...
    if (tmp != nullptr && len > 0) {
      // Проходим по элементам массива
      for (size_t i = 0; i < len; i++) {
        // Если это не ESP на которой выполняется этот скетч
        if (i != NUM) {
          // Обновляем данные массива структур
          data[i].num = tmp[i].num;
          data[i].boardIP = tmp[i].boardIP;
          data[i].mode = tmp[i].mode;
          data[i].sendTo = tmp[i].sendTo;
          data[i].message = tmp[i].message;
          data[tmp->num].RSSI1From = tmp->RSSI1From;
          data[tmp->num].RSSI1 = tmp->RSSI1;
          data[tmp->num].RSSI2From = tmp->RSSI2From;
          data[tmp->num].RSSI2 = tmp->RSSI2;
          data[tmp->num].RSSI3From = tmp->RSSI3From;
          data[tmp->num].RSSI3 = tmp->RSSI3;
        }
      }
    }
  }
}




void setup()
{
  // Инициируем последовательный порт
  Serial.begin(115200);

  if(WiFi.status() != WL_CONNECTED) {
    // Инициируем WiFi
    WiFi.begin("Anchor0", PASSWORD); 
    // Ждём подключения WiFi
    Serial.print("Подключаем к WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(100);
    }
    NUM = WiFi.localIP()[3] - 1;
    Serial.println();
  }

  if(NUM >= 0) {

    data[NUM].RSSI1From = "Anchor0";
    data[NUM].RSSI2From = "Anchor1";
    data[NUM].RSSI3From = "Anchor2";
    
    // Добавляем номер этой платы в массив структур
    data[NUM].num = NUM;
    // Записываем адрес текущей платы в элемент структуры
    data[NUM].boardIP = WiFi.localIP();
    data[NUM].mode = "client";
    
    // Инициируем mDNS с именем "esp32slave" + номер платы
    if (!MDNS.begin(String(slave_host + NUM).c_str())) {
      Serial.println("не получилось инициировать mDNS");
    }
    
    // Узнаём IP адрес платы с UDP сервером
    IPAddress server = MDNS.queryHost(master_host);
    
    // Если удалось подключиться по UDP
    if (udp.connect(server, PORT)) {
    
      Serial.println("UDP подключён");
    
      // вызываем callback функцию при получении пакета
      udp.onPacket(parsePacket);
    }
  }
}




void loop()
{
  if(NUM >= 0 && WiFi.status() == WL_CONNECTED) {

    //Serial.println("scan start");

    // WiFi.scanNetworks will return the number of networks found
    if(WiFi.RSSI()) {
      data[NUM].RSSI1 = WiFi.RSSI();
    } else {
      data[NUM].RSSI1 = 0;
    }
    
    int n = WiFi.scanNetworks();
    //Serial.println("scan done");
    if (n == 0) {
      Serial.println("no networks found");
    } else {
      //Serial.print(n);
      //Serial.println(" networks found");
      for (int i = 0; i < n; ++i) {
        // Print SSID and RSSI for each network found
        if(WiFi.SSID(i) == "SlaveAP_1" || WiFi.SSID(i) == "SlaveAP_2") {
          //Serial.print(i + 1);
          //Serial.print(": ");
          //Serial.print(WiFi.SSID(i));
          //Serial.print(" (");
          //Serial.print(WiFi.RSSI(i));
          //Serial.print(")");
          //Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");

          if(WiFi.SSID(i) == "SlaveAP_1") {
            data[NUM].RSSI2 = WiFi.RSSI(i);
          }
          if(WiFi.SSID(i) == "SlaveAP_2") {
            data[NUM].RSSI3 = WiFi.RSSI(i);
          }
        }
      }
    }
    Serial.println("");
    
  
    // Отправляем данные этой платы побайтово
    Serial.println(data[NUM].RSSI1From);
    Serial.println(data[NUM].RSSI1);
    Serial.println(data[NUM].RSSI2From);
    Serial.println(data[NUM].RSSI2);
    Serial.println(data[NUM].RSSI3From);
    Serial.println(data[NUM].RSSI3);
    
    udp.broadcastTo((uint8_t*)&data[NUM], sizeof(data[0]), PORT);
  
    // Выводим значения элементов в последовательный порт
    //for (size_t i = 0; i < NBOARDS; i++) {
      //Serial.print("Порядковый номер: ");
      //Serial.print(data[i].num);
      //Serial.print(", IP адрес платы: ");
      //Serial.print(data[i].boardIP);
      //Serial.print(", режим работы: ");
      //Serial.print(data[i].mode);
      //if(data[i].mode == "client") {
      //  Serial.print(", RSSI: ");
      //  Serial.println(WiFi.RSSI());
  
        
        /*if(pow(0.1, 2) - pow(pow(10, float(-40-(data[i].RSSI))/(10*2.4)), 2) >= 0) {
          meters = sqrt(pow(0.1, 2) - pow(pow(10, float(-40-(data[i].RSSI))/(10*2.4)), 2));
        } else {
          meters = sqrt(pow(pow(10, float(-40-(data[i].RSSI))/(10*2.4)), 2) - pow(0.1, 2));
        }
        meters = pow(10, float(-43-(data[i].RSSI))/(10*2.4));
        Serial.println(meters);*/
    //}
    Serial.println();
    Serial.println("----------------------------");
  } else {
    Serial.println();
    Serial.println("+-------------------------------------------+");
    Serial.println("| Соединение с WiFi потеряно :(             |");
    Serial.println("| Усердно пытаемся починить ...             |");
    Serial.println("| Проверьте работоспособность MASTER платы. |");
    Serial.println("+-------------------------------------------+");
    Serial.println();
    setup();
  }
  delay(1000);
}
