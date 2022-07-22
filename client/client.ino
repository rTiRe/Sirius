// Подключаем библиотеки
#include "WiFi.h"
#include "AsyncUDP.h"
#include "ESPmDNS.h"

#include <cmath>

#include "Struct.h"



#include <MPU9250_asukiaaa.h>
#include <Wire.h>
#include <GyverTimer.h>
#include <TroykaMQ.h>
#ifdef _ESP32_HAL_I2C_H_
#define SDA_PIN 21
#define PIN_MQ2  32
#define SCL_PIN 22
#endif

#define ACC_UP_H        8.0
#define ACC_DOWN_H      6.0
#define ACC_H           (ACC_UP_H - ACC_DOWN_H)

// Cores
TaskHandle_t Task1;
TaskHandle_t Task2;

MPU9250_asukiaaa mySensor;
MQ2 mq2(PIN_MQ2);

int sensorVal;
int counter;
float aSqrt;
int accState, accStateL;
float acc_max;
bool flag ;
unsigned long t0, t1, t2, t3;
float sq_sqrt;
int piezoPin = 34;
int analogMQ7 = 35;             // Пин к которому подключен A0
int val = 0;
int gas = 0;

const int NUM_READ = 20;




// Определяем количество плат
#define NBOARDS 256

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
          data[i].RSSI1From = tmp[i].RSSI1From;
          data[i].RSSI1 = tmp[i].RSSI1;
          data[i].RSSI2From = tmp[i].RSSI2From;
          data[i].RSSI2 = tmp[i].RSSI2;
          data[i].RSSI3From = tmp[i].RSSI3From;
          data[i].RSSI3 = tmp[i].RSSI3;
          for(int o = 0; o < 8; o++) {
            data[i].APsX[o] = tmp[i].APsX[o];
            data[i].APsY[o] = tmp[i].APsY[o];
          }
        }
      }
    }
  }
}


float median(float newVal) {
  static float buffer[NUM_READ];  // статический буфер
  static byte count = 0;
  buffer[count] = newVal;
  if ((count < NUM_READ - 1) and (buffer[count] > buffer[count + 1])) {
    for (int i = count; i < NUM_READ - 1; i++) {
      if (buffer[i] > buffer[i + 1]) {
        float buff = buffer[i];
        buffer[i] = buffer[i + 1];
        buffer[i + 1] = buff;
      }
    }
  } else {
    if ((count > 0) and (buffer[count - 1] > buffer[count])) {
      for (int i = count; i > 0; i--) {
        if (buffer[i] < buffer[i - 1]) {
          float buff = buffer[i];
          buffer[i] = buffer[i - 1];
          buffer[i - 1] = buff;
        }
      }
    }
  }
  if (++count >= NUM_READ) count = 0;
  return buffer[(int)NUM_READ / 2];
}

// --х--

// вспомогательная функция усреднительного фильтра

// (не очень важно понимать, как она работает, вход функции - реальное значение, выход - отфильтрованное !вход и выход - float!)

float middle(float newVal) {
  static float filVal = 0;
  float k;
  // резкость фильтра зависит от модуля разности значений
  if (abs(newVal - filVal) > 1.5) k = 0.9;
  else k = 0.03;

  filVal += (newVal - filVal) * k;
  return filVal;
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

  accState = 0;
  pinMode(13, OUTPUT);
  pinMode(12, INPUT_PULLUP);
   mq2.calibrate();
  #ifdef _ESP32_HAL_I2C_H_
  // for esp32
  Wire.begin(SDA_PIN, SCL_PIN); 
  //sda, scl
  #else
  Wire.begin();
  #endif
  mySensor.setWire(&Wire);
  mySensor.beginAccel();


  xTaskCreatePinnedToCore(
                    Task1code,   /* Функция задачи */
                    "Task1",     /* Название задачи */
                    10000,       /* Размер стека задачи */
                    NULL,        /* Параметр задачи */
                    1,           /* Приоритет задачи */
                    &Task1,      /* Идентификатор задачи,
                                    чтобы ее можно было отслеживать */
                    0);          /* Ядро для выполнения задачи (0) */                  
  delay(500); 

  // Создаем задачу с кодом из функции Task2code(),
  // с приоритетом 1 и выполняемую на ядре 1:
  xTaskCreatePinnedToCore(
                    Task2code,   /* Функция задачи */
                    "Task2",     /* Название задачи */
                    10000,       /* Размер стека задачи */
                    NULL,        /* Параметр задачи */
                    1,           /* Приоритет задачи */
                    &Task2,      /* Идентификатор задачи,
                                    чтобы ее можно было отслеживать */
                    1);          /* Ядро для выполнения задачи (1) */
  delay(500);
}


void updateAccState(float sqacc)
{
    switch(accState){
        case 0:{
            accState = 1;
            acc_max = -1.;
            accStateL = 0;
        }
        case 1:{    
            if(sqacc > ACC_DOWN_H){
                accState = 2;
                t0 = millis();
            }
            break;
        }
        case 2:{
            if(sqacc > ACC_UP_H){
                accState = 3;
                accStateL = 4;
                t1 = millis();
            }
            else if(sqacc < ACC_DOWN_H){
                accState = 5;
                accStateL = 2;
                t3 = millis();
            }
            break;
        }
        case 3:{
            if(sqacc < ACC_UP_H){
                t2 = millis();
                accState = 4;
            }
            break;
        }
        case 4:{
            if(sqacc < ACC_DOWN_H){
                t3 = millis();
                accState = 5;
            }
            break;
        }
        case 5:{
              /*
            if(accStateL == 2){
                sq_sqrt = 0.5*(t3-t0)*(acc_max - ACC_DOWN_H);
            
                if ()
                Serial.println("Down:"+String(sq_sqrt));
                Serial.println("Down_t0:"+String(t0));
                Serial.println("Down_t3:"+String(t3));
                Serial.println("Разница" + String(t3 - t0));
                Serial.println("Down_acc_max:"+String(acc_max));
            }
            */
            
             if(accStateL == 4){
                sq_sqrt = 0.5*(t2-t1)*(acc_max - ACC_UP_H) + 
                           0.5*(t2-t1)*(t3-t0)*ACC_H;
                float diver = sq_sqrt/(t3-t0); 
                if (diver < 15){

  /*              
                Serial.println("?//////////////////////////////////////////////////");
                Serial.println("Force:"+String(sq_sqrt));
                Serial.println("Force_t0:"+String(t0));
                Serial.println("Force_t1:"+String(t1));
                Serial.println("Force_t2:"+String(t2));
                Serial.println("Force_t3:"+String(t3));
                Serial.println("Разница" + String(t3 - t0));
                Serial.println("Force_acc_max:"+String(acc_max));
                Serial.println("?//////////////////////////////////////////////////");
                counter = 1;

*/
               
                counter = 1;
                Serial.println(counter);
      
                digitalWrite(13, HIGH);
              //  Serial.print("1");
               // Serial.println(sensorVal);
                   while (sensorVal == 1){
                        sensorVal = digitalRead(12);
                   // Serial.print("2");
                   // Serial.println(sensorVal);
              if (sensorVal == 0){
              digitalWrite(13, LOW);
              }
              }

                } 
               else if (diver > 15) {
                
                counter = 2;
                Serial.println(counter);
                digitalWrite(13, HIGH);

              while (sensorVal == 1){
                sensorVal = digitalRead(12);
              if (sensorVal == 0){
              digitalWrite(13, LOW);
              }
              }
            }
            accState = 0;
            break;
        }
        default:{
            accState = 0;
            acc_max = -1.;
            accStateL = 0;
            break;
        }
    }
    
}
}


void Task1code( void * pvParameters ){
  Serial.print("Task1 running on core ");
           //  "Задача Task1 выполняется на ядре "
  Serial.println(xPortGetCoreID());

  for(;;){
    if(NUM >= 0 && WiFi.status() == WL_CONNECTED) {
      if(WiFi.RSSI()) {
        data[NUM].RSSI1 = WiFi.RSSI();
      } else {
        data[NUM].RSSI1 = 0;
      }
      
      int n = WiFi.scanNetworks();
      if (n == 0) {
        Serial.println("no networks found");
      } else {
        Serial.print(data[0].APsX[0]);
        Serial.println(data[0].APsY[0]);
        for (int i = 0; i < n; ++i) {    
          if(WiFi.SSID(i) == "SlaveAP_" + String(data[0].APsX[0])) {
            data[NUM].RSSI2 = WiFi.RSSI(i);
            Serial.println("SlaveAP_" + String(data[0].APsX[0]));
          }
          if(WiFi.SSID(i) == "SlaveAP_" + String(data[0].APsY[0])) {
            data[NUM].RSSI3 = WiFi.RSSI(i);
            Serial.println("SlaveAP_" + String(data[0].APsY[0]));
          }
        }
      }
      Serial.println("");
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
}


void Task2code( void * pvParameters ){
  Serial.print("Task2 running on core ");
           //  "Задача Task2 выполняется на ядре "
  Serial.println(xPortGetCoreID());

  for(;;){
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
  }
}


void loop()
{ 
}
