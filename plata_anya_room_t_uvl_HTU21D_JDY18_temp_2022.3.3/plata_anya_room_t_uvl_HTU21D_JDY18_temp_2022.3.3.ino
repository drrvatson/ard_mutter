/**********************************************************************************************************
Версия 2022.7.1 - год. месяц.номер версии 02/03/2022
Изменения:
-добавил ПИН. который переключает блуту модуль JDY18 в AT-режим
-чистка от комментариев черновиков
-ввел переменную давления для отправки на основную плату
-пробую автоподключение
-подключили блу ту
- подключили модуль 
// AltSoftSerial always uses these pins://
// Board          Transmit  Receive   PWM Unusable
// -----          --------  -------   ------------
// Arduino Uno        9         8         10
************************************************** ************************************************** *****
 Подгружаем библиотеки
/**********************************************************************************************************/
#include "GyverHTU21D.h"
#include "AltSoftSerial.h"  //аналог софтсериал - для работы с блуту модулем

// Определяем константы
/**********************************************************************************************************/
#define MSFB 55           //MAX_SIMVOLOV_FROM_BL - максим кол-во символов с блуту
#define PASSBLUE "99999"  //пароль доступа для подключения через blue
#define PINBLUETODISK 10  //номер ПИН-а
#define PINLED13 13       //номер ПИН-а

/**********************************************************************************************************/
/**********************************************************************************************************
 Определяем переменные
/**********************************************************************************************************/
/*******************************/
//переменные для for blue:
int8_t buffIntAlt;          //считали данные из блуту в инте
int8_t nBlCtd;              //счетчик соединения блуту
char bufferalalt[MSFB];     //буферные данные, полученные от платы блуту
boolean passblue = false;   //metka pass введен
boolean statconnector = false;//подключен ли кто либо
boolean statled = false;    //статус светодиода 13
uint32_t myTimerStat = 0;   //нужен для таймера
uint32_t myTimerPass = 0;   //нужен для таймера
uint32_t timeMetkapass = 0; //определяет когда отключать пользователя. При = 2 идет отключение связи блуту;
boolean connectorOk = false;
boolean blueDataFlag;       //флаг получения данных с блуту платы
uint32_t myTimerStat2 = 0;
String strData = "";
String tanya = "";       //температура из датчика
String panya = "";       //давление из датчика
boolean recievedFlag;       //флаг получения данных
uint32_t timeForPodkl = 0;  //таймер подключения блуту
/**********************************************************************************************************
 объявляем объекты
/**********************************************************************************************************/
AltSoftSerial altSerial;    // инициализируем объект алтсериал для блуту
GyverHTU21D htu;            // библиотека работы с модулем htu21d
/*********************************************************************************************************/
/**********************************************************************************************************
Начало
/**********************************************************************************************************/

void setup() {
  pinMode(PINBLUETODISK, OUTPUT);
  pinMode(PINLED13, OUTPUT);
  Serial.begin(9600);
  htu.begin();          // запустить датчик  
  Serial.begin(9600);
  altSerial.begin(9600);
  nBlCtd = 0;            //обнулили счетчик соединения с блуту. Как только достигает опред значение. происходит сброс

  while (!Serial) { };  // wait for serial port to connect. Needed for native USB port only
//  while (!altSerial) { };  // wait for serial port to connect. Needed for native USB port only
/*  digitalWrite(PINBLUETODISK, HIGH);                     //переводим модуль получения в режим приема AT-команд
  delay(100);
  altSerial.print("AT+DISC\r\n");                       //отлючаем бл. если он был подключен
  delay(100);
  altSerial.print("AT+RESET\r\n");                      //делаем сброс бл
  delay(100);
  digitalWrite(PINBLUETODISK, LOW);  */                  //перевели бл в режим коннектора
}                       //end setup

void loop() {
  /*  while (Serial.available() > 0) {       // ПОКА есть что то на вход с USB-порта    
    strData += (char)Serial.read();        // забиваем строку принятыми данными
    recievedFlag = true;                   // поднять флаг что получили данные
    delay(2);                              // ЗАДЕРЖКА. Без неё работает некорректно!
  }
  if (recievedFlag) {
    Serial.println(strData);               // что пришло на порт. выводится на экран для контроля
    altSerial.print(strData);              // что пришло с порта перенаправляется на порт
    
    strData = "";                          // очистить полученные данные
    recievedFlag = false;                  // опустить флаг, показывающий что пришли данные
  }*/
  /**********************************************************************/
//Работаем с блуту - подключаем, проверяем авторизацию - переписываю код
/**********************************************************************/  

  while (altSerial.available() > 0) {     //смотрим поступало ли что по блу-ту
        buffIntAlt = altSerial.read();//считали данные из блуту в инте
//        Serial.print("buffIntAlt: ");Serial.println(buffIntAlt);
        int lengthBuf = strlen(bufferalalt);//опеределяем заполненность буфера данных из блуту, чтобы не было больше максимального
        //bufferalalt
        if (lengthBuf < MSFB) { //проверка на максимальное значение полученых данных
          if (((buffIntAlt > 47) && (buffIntAlt < 58)) || ((buffIntAlt > 64) && (buffIntAlt < 91)) || ((buffIntAlt > 96) && (buffIntAlt < 123))) //отсеиваем сторонние символы
            {
              bufferalalt[lengthBuf] = (char)buffIntAlt;//записали очередной полученный символ
            }
        }
        blueDataFlag = true; //что-то поступило
        delay(2);
  }
  if (blueDataFlag) { //если данные прилетели из блуту
        Serial.print("Исходник с блуту char: ");Serial.println(bufferalalt);     
        memset(bufferalalt, 0, strlen(bufferalalt)); //я так очищаю буфер, каждую ячейку
 //       delay(20);
        blueDataFlag = false;
 //       delay(10); 
      }
     
/***************************************************************/
//проверяем статус подключения блуту
/***************************************************************/
//statled = !statled;
//digitalWrite(PINLED13, statled = !statled); delay(2500);

if (millis() - timeForPodkl >= 15000) {
      digitalWrite(PINBLUETODISK, HIGH);         //переводим блуту модуль в режим подключения
      altSerial.print("AT+CONN200509210675\r\n");//соед с блуту по Маку 
      delay(2);
     /* if (htu.readTick()) {
        Serial.print("Температура датчика Ани (tanya): ");Serial.println(htu.getTemperature()); delay(50);
        tanya = "tanya=" + String(htu.getTemperature()); //получили температуру из аниной комнаты
        altSerial.println(tanya); delay(50);
        Serial.print("Влажность датчика Ани (panya): ");Serial.println(htu.getHumidity()); delay(50);
        panya = "panya=" + String(htu.getHumidity());
        altSerial.println(panya); delay(50);
      }*/
      nBlCtd = nBlCtd + 1; Serial.print("счетчик бл ");Serial.println(nBlCtd);
      timeForPodkl = millis();
      
}
if (nBlCtd >= 4) {
  delay(500); 
  digitalWrite(PINBLUETODISK, LOW);                     //переводим модуль получения в режим приема AT-команд
  altSerial.print("AT+DISC\r\n");          //Пробуем разорвать соединение
  Serial.println("пробую выкл бл"); 
  nBlCtd = 0;
}

/*if (nBlCtd >= 8) {
  //nBlCtd = 0;
}*/

/***************************************************************/
//конец проверки статуса подключения блуту
/***************************************************************/ 
}
