/************************************************* ************************************************** ******
Этот программа, обслуживающая макет-плату. Показывает на 8 сегм модуль время, температуру. 
Получает от платы управления W5100 время в timestamp, опрашивает датчики и шлет в плату управления w5100
Подключили bluetooth
**
Версия 2022.3.1 - год. месяц.номер версии 10/03/2022
Изменения:
-добавление в получаемый символ знак запятой и точки
-улучшаю код работы с блуту модулем JDY-23:
-вернули JDY-23
-вернули жк-дисплей, он будет выводить данные
-подключили ко второй плате ард, управляющей eth
-макет аккумулирует все блуту модули
************************************************** ************************************************** *****/
/**********************************************************************************************************
 Подгружаем библиотеки
/**********************************************************************************************************/
#include "Wire.h"           //библиотека для работы по шине I2C
#include <SoftwareSerial.h> //библиотека обращения по порту serial
#include "RichUNODS1307.h"  //модуль часов точного времени 1307, работает по шине I2C, адрес 0x68
#include "GyverTM1637.h"    //Управление дисплеем 4 сегмента и точка, светодиодный
#include "RichUNOLM75.h"    //Библиотека датчика температуры
#include "AltSoftSerial.h"  //аналог софтсериал - для работы с блуту модулем
#include "MsTimer2.h"       //прерывание по 2 таймеру - для моргания точек во времени
#include "GyverBME280.h"    //Библиотека работы с модулем BME280 по шине I2C, адрес 0x76
#include "LiquidCrystal_I2C.h"//Библиотека работы с ЖК-дисплеем
#include "UnixTime.h"       //Библиотека конвертации времени в таймстемп и обратно

//#include "softUART.h"       //библиотека для отправки/приема данных от др платы
//#include "GBUS.h"           //библиотека для отправки/приема данных от др платы
/**********************************************************************************************************/
// Определяем константы
/**********************************************************************************************************/
#define CLK 10 //CLK of the TM1637 IC connect to D10 of Arduino
#define DIO 11 //DIO of the TM1637 IC connect to D11 of Arduino
#define MSFB 11 //MAX_SIMVOLOV_FROM_BL - максим кол-во символов с блуту
#define PASSBLUE "99999"  //пароль доступа для подключения через blue

/**********************************************************************************************************
 Определяем переменные
/**********************************************************************************************************/
float celsius_;             //переменная в которую будем записывать данные с темпдатчика
float celsiusT;             //переменная для опроса температуры перед отправкой на плату w5100
int8_t TimeDisp[] = {0x00,0x00,0x00,0x00};//для использования показывания времени на дисплее, этот массив направится в объект disp
unsigned char ClockPoint = 1; //метка для моргания точек в часах
unsigned char time_show = 1;  //метка определяет можно ли отображать точки в часах. Если температура, то нельзя  
uint32_t myTimer1 = 0;        //В эту переменную записывается текущее значение таймера 
//и потом считается разница до 5000мс - DELAY теперь исполтзовать нельзя. СТОП. delay использует TIMER0
//каждые 5сек показывается или время или температура с датчика платы
uint32_t myTimer2 = 0;
uint32_t myTimerSet = 0;        //используется для таймера отправки сообщений на плату управления w5100
uint8_t progr = 1;            //метка определяющая какая подпрограмма должна отработать, 1 - время, 3 - температура
byte massForTemp[] = {0x3f,0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f};//для перевода цифр в соответвующие байт значение для отображения на диспелее
//0x3f - 0,0x06 - 1, 0x5b - 2, 0x4f-3, 0x66-4, 0x6d-5, 0x7d-6, 0x07-7, 0x7f-8, 0x6f-9
float celsiusAltSer;          //сюда будут записываться данные с датчика температуры
//char test[5] = "test";
int8_t showOn = 0;
float pressure;               //давление с датчика BME280
String strData = "";          //получили данные от платы упр eth
String blueData = "";
String strDataOutSet = "";    //подготовка строки отправки данных
boolean recievedFlag;
boolean blueDataFlag;
/*******************************/
//переменные для for blue:
int8_t buffIntAlt;          //считали данные из блуту в инте
char dataAlt[MSFB];         //массив для сохранения данных с алт, индекс - ограничение максим данных
char bufferalalt[MSFB];     //буферные данные, полученные от платы блуту
float floatBluetooth;       //данные полученные с bl и переведенные во float
int8_t intBluetooth;        //данные полученные с bl и переведенные в int
boolean passblue = false;   //metka pass введен
boolean statconnector = false;//подключен ли кто либо
uint32_t myTimerStat = 0;   //нужен для таймера
uint32_t myTimerPass = 0;   //нужен для таймера
uint32_t timeMetkapass = 0; //определяет когда отключать пользователя. При = 2 идет отключение связи блуту;
boolean connectorOk = false;
/*******************************************************/
/******************данные eth-модуля w5100**************/
/*// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
//IPAddress server(74,125,232,128);  // numeric IP for Google (no DNS)
char server[] = "vsroom.ru";    // name address for Google (using DNS)

// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 1, 177);
IPAddress myDns(8, 8, 8, 8);
/*********************************************************************************************************/
/**********************************************************************************************************
 объявляем объекты
/**********************************************************************************************************/
LM75 temper;                // initialize an LM75 object "температуры" for temperature
GyverTM1637 disp(CLK, DIO); // инициализируем объект дисплей
//SoftwareSerial mySerial(8,9); // RX, TX
SoftwareSerial mySerial(5,6); // RX, TX
AltSoftSerial altSerial;    // инициализируем объект алтсериал для блуту
DS1307 clock;               //define a object of DS1307 class - модуль часов точного времени
GyverBME280 bme;            //объект датчика BME280
LiquidCrystal_I2C lcd(0x27, 16, 2);  // адрес, столбцов, строк
UnixTime stamp(3);  // указать GMT (3 для Москвы)

/*********************************************************************************************************/
/**********************************************************************************************************
Начало
/**********************************************************************************************************/
void setup() {
//  Ethernet.init(10);  // Most Arduino shields // You can use Ethernet.init(pin) to configure the CS pin
  // put your setup code here, to run once:
  Serial.begin(9600);
  mySerial.begin(4000);
  while (!Serial) { }; // wait for serial port to connect. Needed for native USB port only
  altSerial.begin(9600);

  Wire.begin();//you should run this function first, so that I2C device can use the I2C bus
  bme.begin(0x76);           // Если доп. настройки не нужны  - инициализируем датчик
  disp.clear();
  disp.brightness(7); // яркость, 0 - 7 (минимум - максимум)
  clock.begin();      // в этой ф-ции запускается объект wire для обращения к модулю точного времени  
  pinMode(13, OUTPUT);
  MsTimer2::set(500, point); // 500ms period запускаес таймер2. Он будет управлять морганием точек в часах
 // MsTimer2::set(5000, blue);
  MsTimer2::start();
  disp.point(1);             // указываем библиотеке включить зажечь точки
//чтобы точки не моргали на пустом дисплее, то сразу отобразим время на дисплее. а иначе 5мс моргают точки
  clock.getTime();           // получаем точное время из модуля точного времени
  TimeDisp[0] = clock.hour / 10;
  TimeDisp[1] = clock.hour % 10;
  TimeDisp[2] = clock.minute / 10;
  TimeDisp[3] = clock.minute % 10;
  disp.display(TimeDisp);


//***********Включаю ЖК-Дисплей****************/
  lcd.init();           // инициализация
  lcd.backlight();      // включить подсветку  
  lcd.setCursor(2, 0);  // столбец 1 строка 0
  lcd.print("Hello, VOVA!");
  lcd.setCursor(6, 1);  // столбец 4 строка 1
  lcd.print("VSCOM");
//***********************************************/

}

void loop() {
  //делаем задержку 5 сек и далее в соттветствии с меткой запускаем очередную программу
  if (millis() - myTimer1 >= 5000) {   // ищем разницу (5 сек)
    myTimer1 = millis();              // сброс таймера
    if(progr == 1){ //метка позать время
      time_show = 1;//показывать точки разрешаю
      TimeUpdate();//функция прописывающая в массив-байт TimeDisp текущее время
      disp.display(TimeDisp);//отображаем время на дисплее
/*      lcd.clear();
      lcd.setCursor(2, 0);  // столбец 1 строка 0
      lcd.print("Hello, VOVA!");
      lcd.setCursor(6, 1);  // столбец 4 строка 1
      lcd.print("VSCOM");*/
      progr = 2;//указываем следующую метку, какая программа должна сработать, что нужно отобразить
    }else if(progr == 2){
      time_show = 0;                    //запрет на моргание точек. при метке 2 выводится температура
      disp.point(POINT_OFF);            //выключаем точки
      celsius_ = (float)temper.getTemperatue();//получаем данные температуры
      //Округления идут правильно. Данные получаю например: 24,496 -> 24,00
//      Serial.print("Температура: ");
//      Serial.println(celsius_);
/*      lcd.clear();
      lcd.setCursor(0, 0);  // столбец 1 строка 0
      lcd.print("Temperatura LM75");
      lcd.setCursor(5, 1);  // столбец 1 строка 0
      lcd.print(celsius_);*/
      displayTemperature_(celsius_);    //выводим температуру на дисплей
      altSerial.print("Температура интегрированного датчика: "); altSerial.println(celsius_);//выводим темп в блуту
      progr = 3;                        //метка следующей подпрограммы после 0,5сек
    }else if(progr == 3){               //выводим данные BME280 датчик темп и давления. 3 - только давление
      time_show = 0;
      pressure = bme.readPressure();    // Читаем давление в [Па] пример: Pressure: 977.34 hPa
      pressure = pressureToMmHg(pressure);//Преобразуем давление в [мм рт. столба] , 733.07 mm Hg
      displayPressure(pressure);        //выводим давление на дисплей
/*      lcd.clear();
      lcd.setCursor(0, 0);  // столбец 1 строка 0
      lcd.print("Davleniye, mmRST");
      lcd.setCursor(5, 1);  // столбец 1 строка 0
      lcd.print(pressure);*/
      altSerial.print("Давление с датчика в комнате: ");
      altSerial.println(pressure);
      progr = 4;
    }else if(progr == 4){               //выводим данные BME280 датчик темп и давления. 4 - только температуры
      time_show = 0;                    //запрет на моргание точек. при метке 4 выводится температура
      disp.point(POINT_OFF);            //выключаем точки
      celsius_ = (float)bme.readTemperature();//получаем данные температуры с BME280
      //Округления идут правильно. Данные получаю например: 24,496 -> 24,00
//      Serial.print("Температура с BME280: ");
//     Serial.println(celsius_);
      altSerial.print("Температура комнаты с внешнего датчика BME280: "); altSerial.println(celsius_);
/*      lcd.clear();
      lcd.setCursor(1, 0);  // столбец 1 строка 0
      lcd.print("Temperatura BME");
      lcd.setCursor(5, 1);  // столбец 1 строка 0
      lcd.print(celsius_);*/
      displayTemperature_(celsius_);    //выводим температуру на дисплей
      progr = 1;
    }
  }
/**********************************************************************/
//Работаем с блуту - подключаем, проверяем авторизацию
/**********************************************************************/  
/*  if (altSerial.available()){     //смотрим поступало ли что по блу-ту
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
        Serial.print("Исходник с блуту char: ");Serial.println(bufferalalt);     
        if (strcmp(bufferalalt, "STAT01") == 0) {statconnector = true;};
        if (strcmp(bufferalalt, "STAT00") == 0) {statconnector = false; passblue = false; connectorOk = false;};
        if (strcmp(bufferalalt, PASSBLUE) == 0) {passblue = true; altSerial.print("passOk");};
        Serial.print("statconnector: ");Serial.println(statconnector);
        Serial.print("passblue: ");Serial.println(passblue);
        if ((strcmp(bufferalalt, "trRoom") == 0) && (connectorOk == true)) {
          celsius_ = (float)temper.getTemperatue();//получаем данные температуры
          altSerial.print("Температура комнаты интегрированного датчика: "); altSerial.println(celsius_);
        }; 
        if (altSerial.available() == 0) {memset(bufferalalt, 0, strlen(bufferalalt));}; //я так очищаю буфер, каждую ячейку
      }
/***************************************************************/
/**********************************************************************/
//Работаем с блуту - подключаем, проверяем авторизацию - переписываю код
/**********************************************************************/  
/*  while (altSerial.available() > 0) {         // ПОКА есть что то на вход
    buffIntAlt = altSerial.read();//считали данные из блуту в инте    
    blueData += (char)mySerial.read();        // забиваем строку принятыми данными
    blueDataFlag = true;                   // поднять флаг что получили данные
    delay(2);                              // ЗАДЕРЖКА. Без неё работает некорректно!
  }
*/  
  while (altSerial.available() > 0) {     //смотрим поступало ли что по блу-ту
        buffIntAlt = altSerial.read();//считали данные из блуту в инте
//        Serial.print("buffIntAlt: ");Serial.println(buffIntAlt);
        int lengthBuf = strlen(bufferalalt);//опеределяем заполненность буфера данных из блуту, чтобы не было больше максимального
        //bufferalalt
        if (lengthBuf < MSFB) { //проверка на максимальное значение полученых данных
          if (((buffIntAlt > 33) && (buffIntAlt < 58)) || (buffIntAlt == 61) || ((buffIntAlt > 64) && (buffIntAlt < 91)) || ((buffIntAlt > 96) && (buffIntAlt < 123))) //отсеиваем сторонние символы
            {
              bufferalalt[lengthBuf] = (char)buffIntAlt;//записали очередной полученный символ
            }
        }
        blueDataFlag = true; //что-то поступило
        delay(2);
  }
  if (blueDataFlag) { //если данные прилетели из блуту
        Serial.print("Исходник с блуту char: ");Serial.println(bufferalalt);     
        if (strcmp(bufferalalt, "STAT01") == 0) {statconnector = true;};
        if (strcmp(bufferalalt, "STAT00") == 0) {statconnector = false; passblue = false; connectorOk = false;};
        if (strcmp(bufferalalt, PASSBLUE) == 0) {passblue = true; altSerial.print("passOk");};
        //обрабатваем данные полученные по блуту
        String tempChToStr = String(bufferalalt);
//        Serial.print("tempChToStr "); Serial.println(tempChToStr);
        int indexLF = tempChToStr.indexOf('=');     //находим есть ли в полученных данных элемент
        if (indexLF != -1) {
          String subStrData1 = tempChToStr.substring(indexLF + 1);
          String subStrData0 = tempChToStr.substring(0,indexLF - 1);
          Serial.print(subStrData0); Serial.println(subStrData1);//поиск переноса строки}
        }
        //конец обработки
        Serial.print("statconnector: ");Serial.println(statconnector);
        Serial.print("passblue: ");Serial.println(passblue);
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
    if (millis() - myTimerStat >= 11000) {
      altSerial.print("AT+STAT\r\n");//так и не понятно. что это и для чего.
      //проверили статус и все. Зачем каждые 11 сек проверять статус подключения к блуту
      //Оказывается этот код надо, чтобы модуль возращал статус, и мы по нему изменяли соответсвующие переменные
      myTimerStat = millis();
    }

      if (millis() - myTimerPass >= 12000) {                     //каждые 12 сек
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
      }

/***************************************************************/
//конец проверки статуса подключения блуту
/***************************************************************/    
/**********************************************/

/*****считываем полученные данные от др платы********/
  while (mySerial.available() > 0) {         // ПОКА есть что то на вход    
    strData += (char)mySerial.read();        // забиваем строку принятыми данными
    recievedFlag = true;                   // поднять флаг что получили данные
    delay(2);                              // ЗАДЕРЖКА. Без неё работает некорректно!
  }
    if (recievedFlag) {                      // если данные получены
    int indexLF = strData.indexOf('\n');     //находим есть ли в полученных данных элемент
    if (indexLF != -1) {
//        Serial.print("indexLF: "); Serial.println(indexLF);//поиск переноса строки}
        String subStrData1 = strData.substring(indexLF + 1);
        String subStrData0 = strData.substring(0,indexLF - 1);
        lcd.clear();
        lcd.setCursor(0, 0);  // столбец 1 строка 0
        lcd.print(subStrData0);
        lcd.setCursor(0, 1);  // столбец 1 строка 0
        lcd.print(subStrData1);
        int indexTS = subStrData0.indexOf("timestamp");//определяем получили ли мы timestamp
        if (indexTS != -1) {
          /********пробуем перевести таймстемп во время****/
          uint32_t TS = subStrData1.toInt();
//          Serial.print("TS = "); Serial.println(TS);
          stamp.getDateTime(TS);
          clock.year = stamp.year;// The clock initial value is set to be Sunday, January 1, 2017
          clock.month = stamp.month;
          clock.dayOfMonth = stamp.day;
          clock.dayOfWeek = stamp.dayOfWeek;
          clock.hour = stamp.hour;// The time initial value is set to 12:30
          clock.minute = stamp.minute;
          clock.second = stamp.second;
          clock.setTime();//write time to the RTC chip записали время.
/*
          // забираем вот так
          Serial.println(stamp.year);
          Serial.println(stamp.month);
          Serial.println(stamp.day);
          Serial.println(stamp.hour);
          Serial.println(stamp.minute);
          Serial.println(stamp.second);
          Serial.println(stamp.dayOfWeek);  // 1 понедельник, 7 воскресенье  */
/************************************************/
          };
//        Serial.print("subStrData: "); Serial.println(subStrData);//поиск переноса строки}
    }else{
        lcd.clear();
        lcd.setCursor(0, 0);  // столбец 1 строка 0
        lcd.print(strData);
    };
//    Serial.println(strData);               // вывести    
    strData = "";                          // очистить
    recievedFlag = false;                  // опустить флаг
  }

  if (millis() - myTimerSet >= 3000000) {
    //    Serial.print("lovi data ");Serial.println("lovi data");
    celsius_ = temper.getTemperatue();//получаем данные температуры
    pressure = bme.readPressure();    // Читаем давление в [Па] пример: Pressure: 977.34 hPa
    pressure = pressureToMmHg(pressure);//Преобразуем давление в [мм рт. столба] , 733.07 mm Hg
    celsiusT = (float)bme.readTemperature();//получаем данные температуры с BME280
    //strDataOutSet = "https://vsroom.ru/?dataput=1&DateTimePlaty=1643351932&TempPlatyLM75=" + String(celsius_) + "&TempBMP280=35&PressureBMP280=760";
    strDataOutSet = String("GET https://vsroom.ru/index.php?dataput=1&TempPlatyLM75=") + celsius_ + "&TempBMP280=" + celsiusT + "&PressureBMP280=" + pressure;
    mySerial.print(strDataOutSet);
    myTimerSet = millis();
  }
  
 /*********************** конец *********************/
};

void point(){                         //вкл и выкл точки на дисплете
  if(time_show == 1){ 
    ClockPoint = !ClockPoint;
    disp.point(ClockPoint);
  }
  digitalWrite(13, !digitalRead(13)); //просто моргает светодиодом
}

void TimeUpdate(void)                 //записываем в байт-массив данные времени
{
  disp.point(1);
  clock.getTime();
  TimeDisp[0] = clock.hour / 10;
  TimeDisp[1] = clock.hour % 10;
  TimeDisp[2] = clock.minute / 10;
  TimeDisp[3] = clock.minute % 10;
}

void displayTemperature_(float temperature)
{
  disp.point(POINT_OFF);
  int8_t tempInt; //полученные данные температуры переводит в int
  byte t0;        //первый байт для вывода на дисплей
  if(temperature < 0) {t0 = 0x40;} else {t0 = 0x00;};//если температура < 0, то ставится -. если >. то НИЧЕГО в первый байт
  temperature = round(temperature);
  tempInt = (int8_t)temperature;
  int8_t t1 = tempInt / 10;  //байт второй
  int8_t t2 = tempInt % 10;  //байт третий
  byte t3 = 0x39;            //байт четвертый для вывода на дисплей - градус Цельсия
  disp.displayByte(t0, massForTemp[t1], massForTemp[t2], t3);  // вывести градус "вручную" побайтово в каждый разряд
}

void displayPressure(float pressure_f){
  disp.point(POINT_OFF);
  int16_t pressure_f_int; //полученные данные температуры переводит в int
  byte t0 = 0x00;        //первый байт для вывода на дисплей
  pressure_f_int = round(pressure_f);
  pressure_f_int = (int16_t)pressure_f;
  byte t1 = pressure_f_int / 100;  //байт второй
  pressure_f_int = pressure_f_int % 100; int8_t t2 = pressure_f_int / 10;  //байт третий
  byte t3 = pressure_f_int % 10;  //байт третий
//  byte t3 = 0x39;            //байт четвертый для вывода на дисплей - градус Цельсия
  disp.displayByte(t0, massForTemp[t1], massForTemp[t2], massForTemp[t3]);  // вывести градус "вручную" побайтово в каждый разряд
  //massForTemp - перевод цифра - в символ вывода на дисплей
}
