#include "ESP8266WiFi.h"
#include "OneWire.h"
//#include "DHT.h"

//int list_wifi();
int get_rssi(String ssid);

//----------- ESP8266------------------
char * your_ssid="VSMIH20";
char * your_pass="SaVoInOne20";
//const char* host = "www.example.com";
IPAddress host(192, 168, 1, 10); //  alien
const int httpPort = 8010;
//----------- DHT11------------------- 
#define DHTPIN D2
#define DHTTYPE DHT11   // DHT 11

//DHT dht(DHTPIN, DHTTYPE);
//---------- DS18B20-------------------
OneWire  ds(D1);  // on pin 10 (a 4.7K resistor is necessary)
//------------------------------------
void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
//    dht.begin();
}

void loop() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));   // Turn the LED on (Note that LOW is the voltage level
    byte i;
    byte present = 0;
    byte type_s;
    byte data[12];
    byte addr[8];
    float celsius, fahrenheit;
    // get data from DHT11
   // float h = dht.readHumidity();     // get humidity
  //  float t = dht.readTemperature();  // get temperature

    // Connect to SSID
    while(!get_rssi(String(your_ssid))) {
      delay(5000);
    }
    WiFi.begin(your_ssid,your_pass);
    while (WiFi.status() != WL_CONNECTED){
        delay(500);
    }
    // get data
    Serial.print("\nConnected, IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC address: ");
    Serial.println(WiFi.macAddress());
    Serial.print("\n");

    if ( !ds.search(addr)) {
        Serial.println("No more addresses.");
        Serial.println();
        ds.reset_search();
        delay(250);
        return;
    }


        // get data from DS18B20 
        Serial.print("ROM =");
        for( i = 0; i < 8; i++) {
            Serial.write(' ');
            Serial.print(addr[i], HEX);
        }

        if (OneWire::crc8(addr, 7) != addr[7]) {
            Serial.println("CRC is not valid!");
            return;
        }
        Serial.println();

        // the first ROM byte indicates which chip
        switch (addr[0]) {
            case 0x10:
                Serial.println("  Chip = DS18S20");  // or old DS1820
                type_s = 1;
                break;
            case 0x28:
                Serial.println("  Chip = DS18B20");
                type_s = 0;
                break;
            case 0x22:
                Serial.println("  Chip = DS1822");
                type_s = 0;
                break;
            default:
              Serial.println("Device is not a DS18x20 family device.");
              return;
        }

        ds.reset();
        ds.select(addr);
        ds.write(0x44, 1);        // start conversion, with parasite power on at the end

        delay(1000);     // maybe 750ms is enough, maybe not
        // we might do a ds.depower() here, but the reset will take care of it.

        present = ds.reset();
        ds.select(addr);
        ds.write(0xBE);         // Read Scratchpad

        Serial.print("  Data = ");
        Serial.print(present, HEX);
        Serial.print(" ");
        for ( i = 0; i < 9; i++) {           // we need 9 bytes
            data[i] = ds.read();
            Serial.print(data[i], HEX);
            Serial.print(" ");
        }
        Serial.print(" CRC=");
        Serial.print(OneWire::crc8(data, 8), HEX);
        Serial.println();

        // Convert the data to actual temperature
        // because the result is a 16 bit signed integer, it should
        // be stored to an "int16_t" type, which is always 16 bits
        // even when compiled on a 32 bit processor.
        int16_t raw = (data[1] << 8) | data[0];
        if (type_s) {
            raw = raw << 3; // 9 bit resolution default
            if (data[7] == 0x10) {
                // "count remain" gives full 12 bit resolution
                raw = (raw & 0xFFF0) + 12 - data[6];
            }
        } else {
            byte cfg = (data[4] & 0x60);
            // at lower res, the low bits are undefined, so let's zero them
            if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
            else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
            else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
            //// default is 12 bit resolution, 750 ms conversion time
        }
        celsius = (float)raw / 16.0;
        fahrenheit = celsius * 1.8 + 32.0;
        Serial.print("  Temperature = ");
        Serial.print(celsius);
        Serial.print(" Celsius, ");
        Serial.print(fahrenheit);
        Serial.println(" Fahrenheit");
        String ds18b20_str=String(celsius);

        // GET request
        WiFiClient client;
        if (client.connect(host, httpPort)) {
            client.print(String("GET /cgi-bin/sensors.cgi?var1=") + ds18b20_str + " HTTP/1.1\r\n" +
                          "Host: " + "\r\n" +
                          "Connection: close\r\n\r\n");
            Serial.println("[Response:]");
            while (client.connected() || client.available()){
                if (client.available()) {
                    String line = client.readStringUntil('\n');
                    Serial.println(line);
                }
            }
            Serial.println("\n[Disconnected]");

            client.stop();
        } else
            Serial.println("connection failed");

        // disconnect
        WiFi.disconnect();
    
    // sleep
    ESP.deepSleep(3e8);    // 5 minutes
}

int get_rssi(String ssid) {
    int ret=0;
    // set wifi mode
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    while (n) {
        if (ssid.equals(WiFi.SSID(--n))) {
          ret=WiFi.RSSI(n);
          break;
        }
    }
  return ret;
}

int list_wifi() {
    // set wifi mode
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    if (n) {
        Serial.print(n);
        Serial.println(" networks found");
        for (int i = 0; i < n; ++i) {
            String ssid=String(i+1) + ": " + WiFi.SSID(i) + " (" + WiFi.RSSI(i) + ")";
            ssid=(WiFi.encryptionType(i) == ENC_TYPE_NONE) ? ssid+"\n" : ssid+"*\n";
            Serial.print(ssid);
            delay(10);
        }
    } else
        Serial.println("no networks found");
    return n;
}
