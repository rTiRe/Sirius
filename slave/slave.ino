  // Подключаем библиотеки
#include <WiFi.h>
#include "AsyncUDP.h"
#include "ESPmDNS.h"

#include "Struct.h"

// Определяем количество плат
#define NBOARDS 4

// Определяем номер этой платы
unsigned int NUM = -1;

// Массив структур для обмена
multidata data[NBOARDS] {0};

int e = 0;
float t = 0;

/* Определяем имена для mDNS */
// для ведущей платы
const char* master_host = "esp32master";
// приставка имени ведомой платы
const char* slave_host = "esp32slave";

// Определяем название и пароль точки доступа
const char* SSID = "Anchor0";
const char* PASSWORD = "Sirius12345";

String SlaveSSID = "";
const char* SlavePASSWORD = "Sirius12345";

// Определяем порт
const uint16_t PORT = 49152;

// Создаём объект UDP соединения
AsyncUDP udp;

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

  WiFi.mode(WIFI_AP_STA);
  
  if(WiFi.status() != WL_CONNECTED) {
    WiFi.begin(SSID, "Sirius12345");
    // Ждём подключения WiFi
    Serial.print("Подключаем к WiFi");
    while (WiFi.status() != WL_CONNECTED && t <= 0.5  ) {
      Serial.print(".");
      t = t + 0.1;
      delay(100);
    }
    if(t > 0.5) {
      if(e > 4) {
        Serial.println("\n\nСлишком долгое подключение, вероятно что-то пошло не так. Поиск остановлен.");
        while(true) {}
      }
      e++;
      Serial.println("\n\nСлишком долго, пробуем снова...\n");
      t = 0;
      setup();
      return;
    }
    if(WiFi.status() == WL_CONNECTED) {
      NUM = WiFi.localIP()[3] - 1;
      Serial.println();
    }
  }

  if(NUM >= 0) {
    // Добавляем номер этой платы в массив структур
    data[NUM].num = NUM;
    // Записываем адрес текущей платы в элемент структуры
    data[NUM].boardIP = WiFi.localIP();
    data[NUM].mode = "slave";
    
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
  data[NUM].sendTo = "master";
  data[NUM].message = "1";
}




void loop()
{
  if(NUM >= 0 && WiFi.status() == WL_CONNECTED) {
  
    // Отправляем данные этой платы побайтово
    udp.broadcastTo((uint8_t*)&data[NUM], sizeof(data[0]), PORT);
  
    // Выводим значения элементов в последовательный порт
    for (size_t i = 0; i < NBOARDS; i++) {
      //Serial.print("Порядковый номер: ");
      //Serial.print(data[i].num);
      //Serial.print(", IP адрес платы: ");
      //Serial.print(data[i].boardIP);
      //Serial.print(", режим работы: ");
      //Serial.print(data[i].mode);
      //if(data[i].mode == "client") {
        //Serial.print(", RSSI: ");
        //Serial.println(data[i].RSSI);
      //}
      //Serial.print("; ");
      //Serial.println();
      if(data[i].mode == "master" && (data[i].message).length() != 0) {
        if(data[i].sendTo == "slave" && data[i].message == "3") {
          data[NUM].message = "3";
          data[NUM].sendTo = "master";
        }
        if(data[NUM].message == "1") {
          SlaveSSID = "SlaveAP_" + data[i].message;
          data[NUM].message = "";
          data[NUM].sendTo = "";
          WiFi.softAP(SlaveSSID.c_str(), SlavePASSWORD);
          WiFi.softAPConfig(IPAddress(192, 168, 4 + (data[i].message).toInt(), 1), IPAddress(192, 168, 4 + (data[i].message).toInt(), 0), IPAddress(255, 255, 255, 0));
          Serial.println(WiFi.softAPIP());
        }
      }
    }
    //Serial.println("----------------------------");
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
 delay(250);
}
