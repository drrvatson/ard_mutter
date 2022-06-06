/**********************************************************************************************************
Версия 2022.3.3 - год. месяц.номер версии 30/03/2022
Изменения:
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
#define MSFB 20 //MAX_SIMVOLOV_FROM_BL - максим кол-во символов с блуту
#define PASSBLUE "99999"  //пароль доступа для подключения через blue
/**********************************************************************************************************/
/**********************************************************************************************************
 Определяем переменные
/**********************************************************************************************************/
/*******************************/
//переменные для for blue:
int8_t buffIntAlt;          //считали данные из блуту в инте
char bufferalalt[MSFB];     //буферные данные, полученные от платы блуту
boolean passblue = false;   //metka pass введен
boolean statconnector = false;//подключен ли кто либо
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
  Serial.begin(9600);
  htu.begin();        // запустить датчик
  
  Serial.begin(9600);

  while (!Serial) { }; // wait for serial port to connect. Needed for native USB port only
  altSerial.begin(9600);
}                    //end setup

void loop() {
/*  // функция опрашивает датчик по своему таймеру
  if (htu.readTick()) {
    // можно забирать значения здесь или в другом месте программы
    Serial.println(htu.getTemperature());
    Serial.println(htu.getHumidity());
    Serial.println();
  }
  delay(2000);*/
    while (Serial.available() > 0) {       // ПОКА есть что то на вход    
    strData += (char)Serial.read();        // забиваем строку принятыми данными
    recievedFlag = true;                   // поднять флаг что получили данные
    delay(2);                              // ЗАДЕРЖКА. Без неё работает некорректно!
  }
  if (recievedFlag) {
    Serial.println(strData);
    altSerial.print(strData);              //так и не понятно. что это и для чего. -
//так понял, что это перенаправление с сериф порта на блуту    
    strData = "";                          // очистить
    recievedFlag = false;                  // опустить флаг
  }
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
 //       if (strcmp(bufferalalt, "STAT01") == 0) {statconnector = true;};
 //       if (strcmp(bufferalalt, "STAT00") == 0) {statconnector = false; passblue = false; connectorOk = false;};
 //       if (strcmp(bufferalalt, PASSBLUE) == 0) {passblue = true; altSerial.print("passOk");};
 //       Serial.print("statconnector: ");Serial.println(statconnector);
 //       Serial.print("passblue: ");Serial.println(passblue);
/*        if ((strcmp(bufferalalt, "trRoom") == 0) && (connectorOk == true)) {
          celsius_ = (float)temper.getTemperatue();//получаем данные температуры
          altSerial.print("Температура комнаты интегрированного датчика: "); altSerial.println(celsius_);
        }; */
        memset(bufferalalt, 0, strlen(bufferalalt)); //я так очищаю буфер, каждую ячейку
        blueDataFlag = false;
      }
/***************************************************************/
//проверяем статус подключения блуту
/***************************************************************/
 /*   if (millis() - myTimerStat >= 11000) {
      altSerial.print("AT+STAT\r\n");//так и не понятно. что это и для чего.
      //проверили статус и все. Зачем каждые 11 сек проверять статус подключения к блуту
      //Оказывается этот код надо, чтобы модуль возращал статус, и мы по нему изменяли соответсвующие переменные
      myTimerStat = millis();
    }*/

/*      if (millis() - myTimerPass >= 12000) {                     //каждые 12 сек
        if (connectorOk == false){
          if ((statconnector == true) && (passblue == false)) {  //смотрим, что к блуту подключились, но что с паролем?
//прошло уже 2 по 12 сек. метка стала 2. Если пароль не прошел, то устройство отключается:
            if (timeMetkapass >= 2) {
              altSerial.print("AT+DISC\r\n"); timeMetkapass = 0; connectorOk = false;}else{
              timeMetkapass++;};
          }
          if ((statconnector == true) && (passblue == true)) {connectorOk = true;};
        }        
        myTimerPass = millis();
      }*/

if (millis() - timeForPodkl >= 10000) {
      if (htu.readTick()) {
        Serial.print("температура у ани ");Serial.println(htu.getTemperature());
        delay(5000);     
      }
      altSerial.print("AT+CONN200509210675\r\n");//соед с блуту по Маку
      delay(5000);
      if (htu.readTick()) {
        tanya = "tanya=" + String(htu.getTemperature());//получили температуру из аниной комнаты
        Serial.print("tempanya: ");Serial.println(tanya);
        altSerial.println(tanya);
        delay(5000);
        panya = "panya=" + String(htu.getHumidity());
        altSerial.println(panya);      
      }
      timeForPodkl = millis();
}

/***************************************************************/
//конец проверки статуса подключения блуту
/***************************************************************/ 
}
