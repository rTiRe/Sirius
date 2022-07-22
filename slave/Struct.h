// Структура данных для обмена

struct multidata {
    // --> Номер платы (необходим для быстрого доступа по индексу в массиве структур
    // --> На клиенте и доп. якорях задается автоматически, на master'е всегда должен быть равен нулю
    uint8_t num;

    // --> IP адрес платы 
    IPAddress boardIP;

    // --> Режим работы платы. Доступные режимы:
    // ----> broker - плата, принимающая и обрабатывающая значения
    // ----> master - основная плата, центральный якорь. Режим автоматически присваивается плате-серверу
    // ----> slave  - вспомогательная плата, якорь, для определения местоположения платы client
    // ----> client - маяк
    String mode;

    // --> mode или конкретный IP адрес
    String sendTo;

    String message;

    // Мощность сигнала
    String RSSI1From;
    String RSSI2From;
    String RSSI3From;
    
    float RSSI1;
    float RSSI2;
    float RSSI3;

    int APsX[8];
    int APsY[8];
};
