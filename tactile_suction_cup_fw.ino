/****************************************************************************************************************************
  RP2040-Client.ino
  For RP2040 with WiFiNINA module/shield.
  
  Based on and modified from Gil Maimon's ArduinoWebsockets library https://github.com/gilmaimon/ArduinoWebsockets
  to support STM32F/L/H/G/WB/MP1, nRF52, SAMD21/SAMD51, RP2040 boards besides ESP8266 and ESP32


  The library provides simple and easy interface for websockets (Client and Server).
  
  Built by Khoi Hoang https://github.com/khoih-prog/Websockets2_Generic
  Licensed under MIT license
 *****************************************************************************************************************************/
/****************************************************************************************************************************
  nRF52 Websockets Client

  This sketch:
        1. Connects to a WiFi network
        2. Connects to a Websockets server
        3. Sends the websockets server a message ("Hello to Server from ......")
        4. Prints all incoming messages while the connection is open

  Hardware:
        For this sketch you only need an nRF52 board.

  Originally Created  : 15/02/2019
  Original Author     : By Gil Maimon
  Original Repository : https://github.com/gilmaimon/ArduinoWebsockets

*****************************************************************************************************************************/

#include "defines.h"

#include <WebSockets2_Generic.h>
using namespace websockets2_generic;

#include <ArduinoJson.h>

#include <SPI.h>

const SPISettings spiSettings(1000000, MSBFIRST, SPI_MODE0);

WebsocketsClient client;

float ch[8];
float voltage;
float pres_voltage;

const int CS_PIN = 10; // ADCのチップセレクトピン
const int CONVST_PIN = 14; // ADCのコンバージョンスタートピン
const int RESET_PIN = 9; // ADCのリセットピン
const int PS_DIGITAL = 8;
const int PS_ANALOG = 15;
const int MOSFET = 7;
const int adc_PIN = 1;  // アナログ入力ピン

byte channel[] = {0x0c, 0xFF};
byte config[] = {0x14, 0x01};


int spi_write(byte* msg){
  digitalWrite(CS_PIN, LOW);
  SPI.transfer(msg, sizeof(msg));
  digitalWrite(CS_PIN, HIGH);
    
  return 0;
}

int set_convst(){
  digitalWrite(CONVST_PIN, HIGH);
  digitalWrite(CONVST_PIN, LOW);
  digitalWrite(CONVST_PIN, HIGH);

  return 0;
}

void onMessageCallback(WebsocketsMessage message)
{
  Serial.print("Got Message: ");
  Serial.println(message.data());

  StaticJsonDocument<256> doc;
  deserializeJson(doc, message.data());
  String msg = doc["msg"];
  deserializeJson(doc, msg);
  int data = doc["data"];

  if (data > 0) {
    digitalWrite(MOSFET, HIGH);
  } else {
    digitalWrite(MOSFET, LOW);
  }

  Serial.print("Received Value: ");
  Serial.println(data);
}

void onEventsCallback(WebsocketsEvent event, String data) 
{
  (void) data;
  
  if (event == WebsocketsEvent::ConnectionOpened) 
  {
    Serial.println("Connnection Opened");
  } 
  else if (event == WebsocketsEvent::ConnectionClosed) 
  {
    Serial.println("Connnection Closed");
  } 
  else if (event == WebsocketsEvent::GotPing) 
  {
    Serial.println("Got a Ping!");
  } 
  else if (event == WebsocketsEvent::GotPong) 
  {
    Serial.println("Got a Pong!");
  }
}

void setup() 
{

  pinMode(CS_PIN, OUTPUT);
  pinMode(CONVST_PIN, OUTPUT);
  pinMode(RESET_PIN, OUTPUT);
  pinMode(PS_DIGITAL, INPUT);
  pinMode(PS_ANALOG, INPUT);
  pinMode(MOSFET, OUTPUT);

  Serial.begin(115200);
  while (!Serial && millis() < 5000);
  
  // initialize network
  Serial.println("\nStarting RP2040-Client with WiFiNINA on " + String(BOARD_NAME));
  Serial.println(WEBSOCKETS2_GENERIC_VERSION);
  
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) 
  {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    return;
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) 
  {
    Serial.println("Please upgrade the firmware");
  }

  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);

  // Connect to wifi
  WiFi.begin(ssid, password);

  // Wait some time to connect to wifi
  for (int i = 0; i < 15 && WiFi.status() != WL_CONNECTED; i++)
  {
    Serial.print(".");
    delay(1000);
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("Connected to Wifi, IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Connecting to WebSockets Server @");
    Serial.println(websockets_server_host);
  }
  else
  {
    Serial.println("\nNo WiFi");
    return;
  }

  // run callback when messages are received
  client.onMessage([&](WebsocketsMessage message) 
  {
    Serial.print("Got Message: ");
    Serial.println(message.data());
  });

  // run callback when messages are received
  client.onMessage(onMessageCallback);

  // run callback when events are occuring
  client.onEvent(onEventsCallback);
  
  // try to connect to Websockets server
  bool connected = client.connect(websockets_server_host, websockets_server_port, "/");
  
  if (connected) 
  {
    Serial.println("Connected!");

    //publish
    //触覚センサ
    for(int i = 0;i < 8;i++){
      StaticJsonDocument<200> doc;
      String WS_msg;
      char topic_name[256];

      doc["op"] = "advertise";
      doc["type"] = "std_msgs/Float32";
      sprintf(topic_name,"/sponge/ch%d", i);
      doc["topic"] = topic_name;
      serializeJson(doc, WS_msg);
      client.send(WS_msg);
    }

    //pres_voltage
    StaticJsonDocument<200> doc;
    String WS_msg;
    char topic_name[256];

    doc["op"] = "advertise";
    doc["type"] = "std_msgs/Float32";
    doc["topic"] = "/sponge/pressure";
    serializeJson(doc, WS_msg);
    client.send(WS_msg);

    //subscribe
    doc.clear();
    doc["op"] = "subscribe";
    doc["topic"] = "/sponge/motor";
    doc["type"] = "std_msgs/Int8";
    serializeJson(doc, WS_msg);
    client.send(WS_msg);
    
  } 
  else 
  {

    Serial.println("Not Connected!");

  }

  SPI.begin(); // SPIの初期化
  SPI.beginTransaction(spiSettings);
  
  digitalWrite(CS_PIN, HIGH); // ADCのチップセレクトピンをHIGHにする
  digitalWrite(CONVST_PIN, HIGH);
  digitalWrite(RESET_PIN, HIGH);
  digitalWrite(MOSFET, LOW);

  digitalWrite(CS_PIN, LOW);
  SPI.transfer(0x0c);
  SPI.transfer(0xff);
  digitalWrite(CS_PIN, HIGH);

  digitalWrite(CS_PIN, LOW);
  SPI.transfer(0x14);
  SPI.transfer(0x01);
  digitalWrite(CS_PIN, HIGH);

  
  // digitalWrite(CS_PIN, HIGH); // ADCのチップセレクトピンをHIGHにする
  // spi_write(channel);
  // spi_write(config);
}

void loop() 
{
  
  // let the websockets client check for incoming messages
  if (client.available()) 
  {
    //MOSFET
    // if(data > 0){
    //   digitalWrite(MOSFET, HIGH);
    // } else {
    //   digitalWrite(MOSFET, LOW);
    // }

    ///////////////////////////////////////////////
    // for(int i = 0;i < 8;i++){
    //   ch[i] = (float)i/10.0;
    // }
    ///////////////////////////////////////////////

    //触覚センサ------------------------------------------
    for(int i = 0; i < 8; i++){
      set_convst();
      delay(0.001);
      digitalWrite(CS_PIN, LOW);
      byte adcValue1 = SPI.transfer(0x00);
      byte adcValue2 = SPI.transfer(0x00);
      digitalWrite(CS_PIN, HIGH);

      Serial.println(adcValue1 >> 5);

      int16_t united_data = (adcValue1 & 0x0f) << 8 | adcValue2;
      
      voltage = (float)(united_data & 0x0fff)*(5.0 / 4096.0);

      ch[i] = voltage;
    }

    //---------------------------------------------------------

    //圧力センサ------------------------------------------------
    int reading = analogRead(adc_PIN);  // アナログ値の読み取り
    pres_voltage = reading*(5.0 / 1023);
    
    //---------------------------------------------------------

    Serial.println(ch[0]);
    Serial.println(ch[1]);
    Serial.println(ch[2]);
    Serial.println(ch[3]);
    Serial.println(ch[4]);
    Serial.println(ch[5]);
    Serial.println(ch[6]);
    Serial.println(ch[7]);

    Serial.print("pres_voltage: ");
    Serial.println(pres_voltage);

    for(int i = 0;i < 8;i++){
      StaticJsonDocument<200> doc, msg;
      String WS_msg;
      char topic_name[256];

      doc["op"] = "publish";
      sprintf(topic_name,"/sponge/ch%d", i);
      doc["topic"] = topic_name;
      msg["data"] = ch[i];
      doc["msg"] = msg;
      serializeJson(doc, WS_msg);
      client.send(WS_msg);
    }

    StaticJsonDocument<200> doc, msg;
    String WS_msg;

    doc["op"] = "publish";
    doc["topic"] = "/sponge/pressure";
    msg["data"] = pres_voltage;
    doc["msg"] = msg;
    serializeJson(doc, WS_msg);
    client.send(WS_msg);

    client.poll();

    // client.send(WS_msg); 
        
  }
  
  //delay(10);
}