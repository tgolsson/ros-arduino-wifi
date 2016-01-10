/* 
 * rosserial Subscriber Example
 * Blinks an LED on callback
 */

#include <ESP8266WiFi.h>

//const char* ssid = "4G-Mobile-WiFi-16D4"; // Your network name here
const char* ssid = "towingle"; // Your network name here
const char* password = "MarxIsGod"; // Your network password here
//const char* ssid = "ASUS"; // Your network name here
//const char* password = "bennyfrost"; // Your network password here
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
//  Serial.print(F("Connecting to WIFI...."));
  WiFi.begin(ssid, password);
  while( WiFi.status() != WL_CONNECTED)
  {
    delay(500);
//    Serial.print(".");
  }

  client.connect(server, 11411);
  client.setNoDelay(nodelay);

//  Serial.println("");
//  Serial.println("WiFi connected");
//  Serial.print("IP address: ");
//  Serial.println(WiFi.localIP());
  Serial.begin(38400);
  newTime = 0;
  oldTime = 0;
}

void loop()
{  
//  newTime = millis();
//  long deltaT = newTime - oldTime;
//  if( deltaT < waitTime)
//  {
//    return;
//  }
//  oldTime = newTime;
//  int readsize = Serial.readBytes(buf, sizeof(buf)-1);
//  buf[readsize] = 0;
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

  int countClient = client.available();
  if(countClient)
  {
    client.read(buf_rec, countClient);
    Serial.write(&buf_rec[0], countClient);
  }
  int countSerial = Serial.available();
  if(countSerial)
  {
    Serial.readBytes(buf_send, countSerial);
    client.write(&buf_send[0], countSerial);
  }
  
//    unsigned int i = 0;
//    char buf[buf_size]; 
//    while(Serial.available() && i < buf_size )
//    {
//      buf[i] = Serial.read();
//      i++;
//    }
//    client.write(&buf[0], i);
//    i = 0;
//
//    unsigned int j = 0;
//    char buf_rec[buf_size];
//    while(client.available() && j < buf_size )
//    {
//      buf_rec[j] = client.read();
//      j++;
//    }
//    Serial.write(&buf_rec[0], j);
//    j = 0;
  
}

