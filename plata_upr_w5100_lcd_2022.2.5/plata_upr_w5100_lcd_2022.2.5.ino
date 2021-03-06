/************************************************* ************************************************** ******
Итоговая программа:
Этот программа управляет платой eth w5100 и lcd
**
Версия 2022.7.15 - год. месяц.номер версии
Изменения:
-изменили данные. получ от платы-макета. - шаблон поменялся
lcd не используем в этой программе.
Подключаем 2 платы вместе
Сперва пробую подключение к w5100
**********************************************************************************************************/
/**********************************************************************************************************
 Подгружаем библиотеки
/**********************************************************************************************************/
#include <SPI.h>            //библиотека работы с SPI
#include <Ethernet.h>       //библиотека с W5100
#include <SoftwareSerial.h> //библиотека обращения по порту serial
#include "softUART.h"       //библиотека для отправки/приема данных от др платы
#include "GBUS.h"           //библиотека для отправки/приема данных от др платы
/**********************************************************************************************************/
// Определяем константы
#define MSFB 120 //
/**********************************************************************************************************/
// Определяем переменные
/**********************************************************************************************************/
/******************данные eth-модуля w5100**************/
// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
//IPAddress server(74,125,232,128);  // numeric IP for Google (no DNS)
char server[] = "vsroom.ru";    // name address for Google (using DNS)

// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 1, 177);
IPAddress myDns(8, 8, 8, 8);
String strData = "";
boolean recievedFlag;
char bufferalalt[MSFB];
int8_t buffIntAlt;
/*********************************************************************************************************/
// Объявляем объекты
/**********************************************************************************************************/
//AltSoftSerial altSerial;    // инициализируем объект алтсериал для блуту
//SoftwareSerial mySerial(8,9); // RX, TX
SoftwareSerial mySerial(8,9); // RX, TX
//LiquidCrystal_I2C lcd(0x27, 16, 2);  // адрес, столбцов, строк
/*********************************************************************************************************/
// Инициализируем объект eth
// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;
// Variables to measure the speed
unsigned long beginMicros, endMicros;
unsigned long byteCount = 0;
bool printWebData = true;  // set to false for better speed measurement
/*********************************************************************************************************/
void setup() {

/***********************************************/
// Open serial communications and wait for port to open:
  Serial.begin(9600);
  mySerial.begin(4000);
  while (!Serial) {;} // wait for serial port to connect. Needed for native USB port only
  delay(2000);
//коннектимся к dhcp
// start the Ethernet connection:
  Serial.println("Initialize Ethernet with DHCP:");
  mySerial.write("Initialize DHCP:");
 delay(2000);
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP"); //lcd.clear(); lcd.setCursor(0, 0);
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
//      lcd.clear(); lcd.setCursor(0, 0); lcd.print("Ethernetnotfound");
      while (true) {
        delay(1); // do nothing, no point running without Ethernet hardware
      }
    }
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
//      lcd.clear(); lcd.setCursor(0, 0); lcd.print("Cable not connec");
    }
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip, myDns);
  } else {
    Serial.print("  DHCP assigned IP ");

    mySerial.print("DHCP assigned IP");
    Serial.println(Ethernet.localIP());

    delay(2000); // do nothing, no point running without Ethernet hardware
//    mySerial.write(strData);
  String strData = DisplayAddress(Ethernet.localIP());  //функция перевода типа IP в String
   Serial.println(strData);
    mySerial.print(strData);
  }
/*************Подключаемся к серверу***************************/
   delay(2000);  
  Serial.print("connecting to "); 
  String strDataSer = "Connecting to \n" + String(server);
//  mySerial.print("Connecting to"); delay(2000);//lcd.clear(); lcd.setCursor(0, 0); lcd.print("Connecting to ");
  mySerial.print(strDataSer); delay(2000);
  Serial.print(server); //mySerial.print(server); delay(2000);//lcd.setCursor(0, 1); lcd.print(server);
  Serial.println("...");
  // if you get a connection, report back via serial:
  if (client.connect(server, 80)) {
    Serial.print("connected to "); //mySerial.print("Connected"); delay(2000);
    String strData_ = "connected to \n" + DisplayAddress(client.remoteIP());  //функция перевода типа IP в String
    mySerial.print(strData_); delay(2000);
    Serial.println(client.remoteIP());
    // Make a HTTP request:
    client.println("GET https://vsroom.ru/index.php?getDataTime=1 HTTP/1.1");
    client.println("User-Agent: ARDUINO");  //добавил агента. чтобы сервер не делел 301-перенаправление
    client.println("Host: vsroom.ru");
    client.println("Connection: close");
    client.println();
  } else {
    // if you didn't get a connection to the server:
    Serial.println("connection failed");
  }

while(client.connected()) 
{
//  lcd.clear(); lcd.setCursor(0, 0); lcd.print("Connected ");
  delay(1000);
   if(client.available())
   {
      String c = client.readString(); //Serial.println(c);
//      Serial.println(c);  
      int val = c.indexOf("getDataTime=");
      if(c.indexOf("getDataTime=") >0) //indexOf или contains
      {
          //тут условия для выполнения действий Ардуиной, если найдено OK=1
          //Serial.print("val_getDataTime="); Serial.println(val);
          String itogStr = "timestamp: \n" + c.substring(227, 237); //получили timestamp
          mySerial.print("Get data:"); delay(2000); mySerial.print(itogStr); delay(2000);
       }
   }

    }
/**************************************************************/
  delay(5000);
}               //end setup

void loop() {

while (mySerial.available() > 0) {
    buffIntAlt = mySerial.read();
    int lengthBuf = strlen(bufferalalt);    //опеределяем заполненность буфера данных из блуту, чтобы не было больше максимального
    if (lengthBuf < MSFB) {
      if (((buffIntAlt > 33) && (buffIntAlt < 58)) || (buffIntAlt == 61) || ((buffIntAlt > 64) && (buffIntAlt < 91)) || ((buffIntAlt > 96) && (buffIntAlt < 123))) //отсеиваем сторонние символы
         {
           bufferalalt[lengthBuf] = (char)buffIntAlt;//записали очередной полученный символ
         }
    }
    recievedFlag = true; //что-то поступило
    delay(2);
}

    if (recievedFlag) {                      // если данные получены
    if (client.connect(server, 80)) {
    strData = String(bufferalalt);
    strData = "GET https://vsroom.ru/index.php?dataput=1" + strData + " HTTP/1.1"; 
    Serial.println(strData);               // вывести   
    client.println(strData);
    client.println("Host: vsroom.ru");
    client.println("User-Agent: ARDUINO");
    client.println("Connection: close");
    client.println(); 
    strData = "";                          // очистить
    recievedFlag = false;                  // опустить флаг
    }else{Serial.println("connection failed");};
    if(client.available())
   {
      String c = client.readString(); //Serial.println(c);
      Serial.println(c);
   }
   memset(bufferalalt, 0, strlen(bufferalalt)); //я так очищаю буфер, каждую ячейку
   recievedFlag = false;
  }
}

String DisplayAddress(IPAddress address)
{
 return String(address[0]) + "." + 
        String(address[1]) + "." + 
        String(address[2]) + "." + 
        String(address[3]);
}
