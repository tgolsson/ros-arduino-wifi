/* 
 * May be freely distributed.
 */

#include <ESP8266WiFi.h>

const char* ssid = "xxxx"; // Your network name here
const char* password = "xxxx"; // Your network password here
IPAddress server(192,168,8,101);

const bool nodelay = false;
long long newTime, oldTime;
double waitTime =  1.0 / 30.0 * 1e3;
const int buf_size = 128;
byte buf_rec[buf_size];
byte buf_send[buf_size];


WiFiClient client;

void setup()
{ 
    delay(3000);
  
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while( WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }

    client.connect(server, 11411);
    client.setNoDelay(nodelay);

    Serial.begin(38400);
    newTime = 0;
    oldTime = 0;
}

void loop()
{  

    // Check connection status
    if( WiFi.status() != WL_CONNECTED)
    {
        while( WiFi.status() != WL_CONNECTED)
        {
            WiFi.begin(ssid, password);
        }
    }
    if( !client.connected() )
    {
        client.stop();
        client.connect(server, 11411);
        client.setNoDelay(nodelay);
    }

    // Read from wifi, write to serial
    int countClient = client.available();
    if(countClient)
    {
        client.read(buf_rec, countClient);
        Serial.write(&buf_rec[0], countClient);
    }

    // Read from serial, write to wifi
    int countSerial = Serial.available();
    if(countSerial)
    {
        Serial.readBytes(buf_send, countSerial);
        client.write(&buf_send[0], countSerial);
    }
}

