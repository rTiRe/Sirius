#include <WiFi.h>
#include "AsyncUDP.h"
#include "ESPmDNS.h"

#include "Struct.h"

// Определяем количество плат
#define NBOARDS 256

/*   ------------------------   */
     String wMode = "slaveX";
     String msg;
/*   ------------------------   */

unsigned int NUM = -1;
multidata data[NBOARDS] {0,};

int e = 0;
float t = 0;

/* Определяем имена для mDNS */
// для ведущей платы
const char* master_host = "esp32master";
// приставка имени ведомой платы
const char* slave_host = "esp32slave";
const char* SSID = "Anchor0";
const char* PASSWORD = "Sirius12345";

String SlaveSSID = "";
const char* SlavePASSWORD = "Sirius12345";
const uint16_t PORT = 49152;
AsyncUDP udp;
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
          data[i].RSSI1From = tmp[i].RSSI1From;
          data[i].RSSI1 = tmp[i].RSSI1;
          data[i].RSSI2From = tmp[i].RSSI2From;
          data[i].RSSI2 = tmp[i].RSSI2;
          data[i].RSSI3From = tmp[i].RSSI3From;
          data[i].RSSI3 = tmp[i].RSSI3;
          data[i].APsX[8] = tmp[i].APsX[8];
          Serial.println(data[0].APsX[0]);
          data[i].APsY[8] = tmp[i].APsY[8];
        }
      }
    }
  }
}




void setup()
{
  Serial.begin(115200);

  WiFi.mode(WIFI_AP_STA);
  if(WiFi.status() != WL_CONNECTED) {
    WiFi.begin(SSID, "Sirius12345");
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
      udp.onPacket(parsePacket);
    }
  }
  data[NUM].sendTo = "master";
  if(wMode == "slaveX") { msg = "0"; }
  if(wMode == "slaveY") { msg = "1"; }
  data[NUM].message = msg;
}

void loop()
{
  if(NUM >= 0 && WiFi.status() == WL_CONNECTED) {
    // Отправляем данные этой платы побайтово
    udp.broadcastTo((uint8_t*)&data[NUM], sizeof(data[0]), PORT);
    for (size_t i = 0; i < NBOARDS; i++) {
      if(data[i].mode == "master" && (data[i].message).length() != 0 && data[i].sendTo == (String(data[NUM].boardIP[0]) + "." + String(data[NUM].boardIP[1]) + "." + String(data[NUM].boardIP[2]) + "." + String(data[NUM].boardIP[3]))) {
        if(data[NUM].message == msg) {
          SlaveSSID = "SlaveAP_" + String(data[NUM].num);
          data[NUM].message = "";
          data[NUM].sendTo = "";
          WiFi.softAP(SlaveSSID.c_str(), SlavePASSWORD);
          WiFi.softAPConfig(IPAddress(192, 168, 4 + NUM, 1), IPAddress(192, 168, 4 + NUM, 0), IPAddress(255, 255, 255, 0));
          Serial.println(WiFi.softAPIP());
        }
      }
    }
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
