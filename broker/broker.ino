// Подключаем библиотеки
#include <WiFi.h>
#include <DNSServer.h>
#include "AsyncUDP.h"
#include "ESPmDNS.h"

#include "Struct.h"

#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>
#include <vector>
#include <fstream>
#include <cmath>
#include <algorithm>

using namespace std;



// Определяем количество плат
#define NBOARDS 256

const char uuid[]={"2702fdbd-d32f-4c26-94c6-4e866ed228d0\0"};

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
const char* slave_host = "esp32broker";

// Определяем название и пароль точки доступа
const char* SSID = "Anchor0";
const char* PASSWORD = "Sirius12345";

// Определяем порт
const uint16_t PORT = 49152;

IPAddress apIP(192, 168, 1, 4);
DNSServer dnsServer;
const char *server_name = "www.myesp32.com";


String responseHTML = "";


// Создаём объект UDP соединения
AsyncUDP udp;
WiFiServer server(80);


struct Package {
  int version;
  char uuid[100];
  float x;
  float y;
  int charge_level;
  int temperature;
  int gas;
  char fall;
  char hit;
  char fire;
  char low_energy;
};
Package pack;

struct point
{
    double x, y;
};
struct trit_t
{
    double d0, d1, d2;
};

point result;

//Фильтр Калмана заголовок функции
double KALMAN(double U);
// U - это измерение (коэффициент) шума
// U_hat - это отфильтрованная оценка

//функция Калмана значения
double KALMAN0(double newVal) {
    double k = 0.1;  // коэффициент фильтрации, 0.0-1.0
// бегущее среднее
  static double filVal = 0;
  filVal += (newVal - filVal) * k;
  return filVal;
}

double KALMAN1(double newVal) {
    double k = 0.1;  // коэффициент фильтрации, 0.0-1.0
// бегущее среднее
  static double filVal = 0;
  filVal += (newVal - filVal) * k;
  return filVal;
}

double KALMAN2(double newVal) {
    double k = 0.1;  // коэффициент фильтрации, 0.0-1.0
// бегущее среднее
  static double filVal = 0;
  filVal += (newVal - filVal) * k;
  return filVal;
}


double rad1(double data)
{
    double h = -29;
    double a = 2;
    double d;
    if(pow(0.1, 2) - pow(pow(10, float(h-(data))/(10*a)), 2) >= 0) {
      d = sqrt(pow(0.1, 2) - pow(pow(10, float(h-(data))/(10*a)), 2));
    } else {
      d = sqrt(pow(pow(10, float(h-(data))/(10*a)), 2) - pow(0.1, 2));
    }
    
    return d;

    /*double h = -69;
    double p = abs((h - (data)) / (10. * 3.));
    double d = pow(10, p);
    return d;*/
}
double rad2(double data, double h, double a) {
  double d;
  if(pow(0.1, 2) - pow(pow(10, float(h-(data))/(10*a)), 2) >= 0) {
    d = sqrt(pow(0.1, 2) - pow(pow(10, float(h-(data))/(10*a)), 2));
  } else {
    d = sqrt(pow(pow(10, float(h-(data))/(10*a)), 2) - pow(0.1, 2));
  }
  
  return d;
}

double norm(point p) // get the norm of a vector
{
    return pow(pow(p.x, 2) + pow(p.y, 2), .5);
}

point trilateration(point point1, point point2, point point3, double r1, double r2, double r3)
{
    point resultPose;
    // unit vector in a direction from point1 to point 2
    double p2p1Distance = pow(pow(point2.x - point1.x, 2) + pow(point2.y - point1.y, 2), 0.5);
    point ex = {(point2.x - point1.x) / p2p1Distance, (point2.y - point1.y) / p2p1Distance};
    point aux = {point3.x - point1.x, point3.y - point1.y};
    // signed magnitude of the x component
    double i = ex.x * aux.x + ex.y * aux.y;
    // the unit vector in the y direction.
    point aux2 = {point3.x - point1.x - i * ex.x, point3.y - point1.y - i * ex.y};
    point ey = {aux2.x / norm(aux2), aux2.y / norm(aux2)};
    // the signed magnitude of the y component
    double j = ey.x * aux.x + ey.y * aux.y;
    // координаты
    double x = (pow(r1, 2) - pow(r2, 2) + pow(p2p1Distance, 2)) / (2 * p2p1Distance);
    double y = (pow(r1, 2) - pow(r3, 2) + pow(i, 2) + pow(j, 2)) / (2 * j) - i * x / j;
    // конечные координаты
    double finalX = point1.x + x * ex.x + y * ey.x;
    double finalY = point1.y + x * ex.y + y * ey.y;
    resultPose.x = finalX;
    resultPose.y = finalY;
    return resultPose;
}

point fin(float RSSI1, float RSSI2, float RSSI3)
{
    vector<trit_t> kalman_res;

    point finalPose;
    point p1 = {0.0, 0.0};
    point p2 = {5.756, 0.0};
    point p3 = {0.0, 6.0};

    //начнем фильтровать все значения шумов
    //инициализируем пустые array для отфильтрованных значений
    //используем фильтр Калмана
    double filterRssi0 = KALMAN0(RSSI1),
           filterRssi1 = KALMAN1(RSSI2),
           filterRssi2 = KALMAN2(RSSI3);

    trit_t a = {rad1(filterRssi0),
                rad1(filterRssi1),
                rad1(filterRssi2)};

    Serial.println(a.d0);
    Serial.println(a.d1);
    Serial.println(a.d2);
                
    kalman_res.clear();
    kalman_res.push_back(a);
    
    finalPose = trilateration(p1, p2, p3, kalman_res[0].d0, kalman_res[0].d1, kalman_res[0].d2);
    //finalPose.x
    //finalPose.y
    /*ofstream myfile;
    myfile.open("./out.csv", ios::out | ios::trunc);
    for (int ch = 0; ch < n; ch++)
    {
        finalPose = trilateration(p1, p2, p3, kalman_res[ch].d0, kalman_res[ch].d1, kalman_res[ch].d2);
        cout << endl;
        cout << setw(7) << setprecision(3) << finalPose.x << " " << finalPose.y << fixed;
        myfile << setw(7) << setprecision(3) << finalPose.x << " " << finalPose.y << fixed << endl;
    }
    myfile.close();*/

    return finalPose;
}



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
          //Serial.println(String(tmp[i].num));
          //Serial.println(IPAddress(tmp[i].boardIP));
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
          data[i].APsY[8] = tmp[i].APsY[8];
        }
      }
    }
  }
}




void setup()
{ 
  pack.version = 0;
  strncpy(pack.uuid, uuid, 37);
  pack.x = 0.0;
  pack.y = 0.0;
  pack.charge_level = 99;
  pack.temperature = 0;
  pack.gas = 0;
  pack.fall = '1';
  pack.hit = '1';
  pack.fire = '0';
  pack.low_energy = '1';

  
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
      WiFi.softAP("BROKER", "SirSinara");

      WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

      server.begin();
    }
  }

  if(NUM >= 0) {
    // Добавляем номер этой платы в массив структур
    data[NUM].num = NUM;
    // Записываем адрес текущей платы в элемент структуры
    data[NUM].boardIP = WiFi.localIP();
    data[NUM].mode = "broker";
    
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
    WiFiClient client = server.available();   // Listen for incoming clients
    if (client) {
      String currentLine = "";   // make a String to hold incoming data from the client
      while (client.connected()) {  // loop while the client's connected
         if (client.available()) {        // if there's bytes to read from the client,
          char c = client.read();      // read a byte, then
          Serial.write(c);                // print it out the serial monitor
          if (c == '\n') {                 // if the byte is a newline character
            // if the current line is blank, you got two newline characters in a row.
            // that's the end of the client HTTP request, so send a response:
            if (currentLine.length() == 0) {
              // Send header
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println("Connection: close");
              client.println();
              
              // Display the HTML web page
              //client.println(arrOfChar,BIN);
              for (size_t i = 0; i < NBOARDS; i++) {
                if(data[i].mode == "client") {
                  //Serial.println("----------------------------");
                  result = fin(data[i].RSSI1, data[i].RSSI2, data[i].RSSI3);
                  pack.x = result.x;
                  pack.y = result.y;
                }
              }

              udp.broadcastTo((uint8_t*)&data[NUM], sizeof(data[0]), PORT);
              client.write((byte*)&pack, sizeof(Package));            
              // The HTTP response ends with another blank line
              client.println();
              break;
            } else { // if we got a newline, then clear currentLine
              currentLine = "";
            }
          } else if (c != '\r') {  // if we got anything else but a CR character,
            currentLine += c;   // add it to the end of the currentLine
          }
        }
      }
      client.stop();
    }

    /*udp.broadcastTo((uint8_t*)&data[NUM], sizeof(data[0]), PORT);
    // Выводим значения элементов в последовательный порт
    for (size_t i = 0; i < NBOARDS; i++) {
      if(data[i].mode == "client") {

        //pack.rssi0 = data[i].RSSI1;
        //pack.rssi1 = data[i].RSSI2;
        //pack.rssi2 = data[i].RSSI3;
        
        /*Serial.print("Порядковый номер: ");
        Serial.print(data[i].num);
        Serial.print(", IP адрес платы: ");
        Serial.print(data[i].boardIP);
        Serial.print(", режим работы: ");
        Serial.print(data[i].mode);
        Serial.print(", message: ");
        Serial.print(data[i].message);
        //Serial.print("RSSI1: ");
        /*Serial.print(data[i].RSSI1);
        Serial.print(", ");
        Serial.print(data[i].RSSI2);
        Serial.print(", ");
        Serial.print(data[i].RSSI3);
        Serial.println();
        //Serial.println(";");
        result = fin(data[i].RSSI1, data[i].RSSI2, data[i].RSSI3);
        Serial.println(String(result.x) + " " + String(result.y) + "\n");
        Serial.println(rad1(data[i].RSSI1));
        Serial.println(rad1(data[i].RSSI2));
        Serial.println(rad1(data[i].RSSI3));
      }
    }
    Serial.println("----------------------------");*/
    
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
 delay(5000);
}
