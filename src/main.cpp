#include <Arduino.h>
#include <M5Atom.h>
#include <WiFi.h>
#include <HttpClient.h>
#include <WebServer.h>
#include <Arduino_JSON.h>
#include "FS.h"
#include <SPIFFS.h>
#include <time.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// import Application Define Config
#include "DefineConfig.h"

// import Ctl module
#include "MistResonater.h"
#include "SunshineLed.h"
#include "ThunderboltLed.h"
#include "WaterSupplyPomp.h"

// Boot Mode String
String modeStr[] = {"SETUP", "DEFAULT"};

// this flag that allows operation in the selected mode.
boolean isReady = false;

// Button Coler Defined
const int32_t red = 0x004000;    // Error
const int32_t yellow = 0x404000; // setup mode
const int32_t green = 0x300000;  // wifi connected
const int32_t blue = 0x000040;   // device starting
const int32_t white = 0x202020;  // Not Asssign

// Digital I/O PIN
int out1 = 22;
int out2 = 19;
int out3 = 23;
int out4 = 33;

// instance
SunshineLed sunshineLed(out1);
MistResonater mistResonater(out2);
WaterSupplyPomp waterPomp(out3);
ThunderboltLed thunderLed(out4);

// wifi setting file
const String wifi_settings = "/wifi_settings.txt";
String mac;

// NTP setting
WiFiUDP ntpUDP;

// SETUP API
String url_path = "/api/v1/setup/wifi";

// http and mqtt client setting
String baseTopic = "$aws/things/aws-arduino-iot/shadow/update/";
const char *pubTopic;
const char *subTopic;
//BearSSL::WiFiClientSecure httpsClient;
WiFiClientSecure httpsClient;
PubSubClient mqttClient(httpsClient);

//server setting
WebServer server(80);

// Use WiFiClient class to create TCP connections
WiFiClient client;

/**
 * Read file
 */
void writeFile(const String file, const String str)
{
  File f = SPIFFS.open(file, FILE_WRITE);
  f.println(str);
  f.close();
}

/** 
 * Write file 
 */
String readFile(const String file)
{
  String str;
  File f = SPIFFS.open(file, FILE_READ);
  str = f.readString();
  f.close();

  return str;
}

/**
 * split String by delimiter
 */
int split(String data, char delimiter, String *dst)
{
  int index = 0;
  int arraySize = (sizeof(data) / sizeof((data)[0]));
  int datalength = data.length();

  for (int i = 0; i < datalength; i++)
  {
    char tmp = data.charAt(i);
    if (tmp == delimiter)
    {
      index++;
      if (index > (arraySize - 1))
        return -1;
    }
    else
      dst[index] += tmp;
  }
  return (index + 1);
  ;
}

/** 
 * Try Connection Check  
 */
boolean tryConnection(const char *id, const char *pw)
{
  boolean wlConnected = false;
  int MAX = CONNECTION_TIMEOUT / RETRY_INTERVAL;

  WiFi.begin(id, pw);

  // wifi 接続待機
  Serial.print("[");
  int n = 0;
  while (n < MAX)
  {
    wlConnected = WiFi.status() == WL_CONNECTED;

    if (wlConnected)
    {
      // wifi configに書き込み
      const String str =
          String(id) + "\n" + String(pw);
      writeFile(wifi_settings, str);

      break;
    }

    delay(RETRY_INTERVAL);
    Serial.print("*");
    n++;
  }
  Serial.println("]");
  return wlConnected;
}

/**
 * synchronized NTP Server
 */
void syncNTPClient()
{

  // NTP 同期
  Serial.println("NTP Sync begin.");
  configTime(TIMEZONE_OFFSET, DAYLIGHT_OFFSET, NTP_SERVER1, NTP_SERVER2);

  // NTP 同期待ち
  Serial.print("[");
  time_t timeNow = time(NULL);
  while (timeNow <= BASE_UNIX_TIME)
  {
    delay(1000);
    timeNow = time(NULL);
    Serial.print("*");
  }
  Serial.println("]");

  struct tm *tm = localtime(&timeNow);
  static const char *wd[7] = {"Sun", "Mon", "Tue", "Wed", "Thr", "Fri", "Sat"};

  Serial.print("Device Time: ");
  Serial.printf(" %04d/%02d/%02d(%s) %02d:%02d:%02d\n",
                tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                wd[tm->tm_wday],
                tm->tm_hour, tm->tm_min, tm->tm_sec);
}

/**
 * MQTTのエラーを表示する
*/
void mqttErrorReport(){
  Serial.print("Failed. Error = ");

  switch (mqttClient.state()) {
    case MQTT_CONNECT_UNAUTHORIZED:
      Serial.println("MQTT_CONNECT_UNAUTHORIZED");
      break;
    case MQTT_CONNECT_BAD_CREDENTIALS:
      Serial.println("MQTT_CONNECT_BAD_CREDENTIALS");
      break;
    case MQTT_CONNECT_UNAVAILABLE:
      Serial.println("MQTT_CONNECT_UNAVAILABLE");
      break;
    case MQTT_CONNECT_BAD_CLIENT_ID:
      Serial.println("MQTT_CONNECT_BAD_CLIENT_ID");
      break;
    case MQTT_CONNECT_BAD_PROTOCOL:
      Serial.println("MQTT_CONNECT_BAD_PROTOCOL");
      break;
    case MQTT_CONNECTED:
      Serial.println("MQTT_CONNECTED");
      break;
    case MQTT_DISCONNECTED:
      Serial.println("MQTT_DISCONNECTED");
      break;
    case MQTT_CONNECT_FAILED:
      Serial.println("MQTT_CONNECT_FAILED");
      break;
    case MQTT_CONNECTION_LOST:
      Serial.println("MQTT_CONNECTION_LOST");
      break;
    case MQTT_CONNECTION_TIMEOUT:
      Serial.println("MQTT_CONNECTION_TIMEOUT");
      break;
  }
}

/**
 * connenct AWS IoT
 */
void connectAWSIoT()
{
  while (!mqttClient.connected())
  {
    if (mqttClient.connect(""))
    {
      Serial.println("MQTT Server Connected.");
      mqttClient.subscribe(subTopic, QOS);
      Serial.println("Subscribed.");
    }
    else
    {
      mqttErrorReport();
      M5.dis.drawpix(0, red);
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/* 
 * Todo: メッセージを引数で受け取りPublishする.
 * @param JsonVar
 * @return
 */
long messageSentAt = 0;
int value = 0;
char pubMessage[MQTT_MAX_PACKET_SIZE];
unsigned long now;
void publishTopic(String message)
{

  ++value;

  snprintf(pubMessage, MQTT_MAX_PACKET_SIZE, message.c_str(), value);
  Serial.print("Publishing. topic= ");
  Serial.println(pubTopic);
  Serial.println(pubMessage);
  mqttClient.publish(pubTopic, pubMessage);

  Serial.println("Published.");
}

/**
 * mptt callback
 */
int delaySec = 36;
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Received. topic= ");
  Serial.println(topic);

  String res;
  for (int i = 0; i < length; i++)
  {
    res += (char)payload[i];
  }
  Serial.println(res);

  // convert to json object
  JSONVar obj = JSON.parse(res);

  // get weather data
  if(obj.hasOwnProperty("weather")) {
    
    JSONVar weathers = obj["weather"];

    for(uint8_t i = 0; i < weathers.length(); i++){
      JSONVar weather = weathers[i];

      String main  =  (const char*)weather["weather"];
      long delaySec = (const int)weather["delay"];

      Serial.println("weather: " + main);
      Serial.print("delay: ");
      Serial.println(delaySec);

      // // module initialized
      if(main.equals("Sunny")) {
        Serial.println("天気は晴れです。");
        sunshineLed.init(delaySec);

      }
      else if(main.equals("Clouds")) {
        Serial.println("天気は曇りです。");
        mistResonater.init(delaySec);

      }
      else if(main.equals("Rainy")) {
        Serial.println("天気は雨です。");
        waterPomp.init(delaySec);

      } else if(main.equals("Thunderstorm")) {
        // Todo: Ledの発行に工夫が必要なため保留(モジュールSSRの選定のみ)
        Serial.println("天気は雷雨です。");
        waterPomp.init(delaySec);
        thunderLed.init(delaySec);

      } else {
        Serial.println("天気の生成に失敗しました。");
      }

      // モジュールタスクの実行
      // if(!sunshineLed.getTaskState()) sunshineLed.task();
      // if(!mistResonater.getTaskState()) mistResonater.task();
      // if(!waterPomp.getTaskState()) waterPomp.task();
      // if(!thunderLed.getTaskState()) thunderLed.task();

      // execute module tasks
      boolean isExecuted = true;
      while(isExecuted) {
  
        // モジュールタスクの実行
        if(!sunshineLed.isDone()) sunshineLed.task();
        if(!mistResonater.isDone()) mistResonater.task();
        if(!waterPomp.isDone()) waterPomp.task();
        if(!thunderLed.isDone()) thunderLed.task();

        isExecuted = !(sunshineLed.isDone()
                            || mistResonater.isDone()
                            || waterPomp.isDone()
                            || thunderLed.isDone());

        Serial.println(sunshineLed.isDone());
        Serial.println(mistResonater.isDone());
        Serial.println(waterPomp.isDone());
        Serial.println(thunderLed.isDone());

        Serial.println(isExecuted);
        delay(500);

      }
    }

    // Received Response
    JSONVar response;
    response["id"] = (const char*)obj["id"];
    publishTopic(JSON.stringify(response));
  }
  
  Serial.println("callback end...");
}

JSONVar obj;
void mqttLoop()
{
  if (!mqttClient.connected())
  {
    connectAWSIoT();
  }
  mqttClient.loop();
}

/**
 * setup mqtt
 */
void initMqtt(uint8_t *mac0, int size)
{

  for (int i = 0; i < size; i++)
  {
    mac += String(mac0[i], HEX);
    if (i < size - 1)
    {
      mac += "-";
    }
  }

  //publish topic
  String pubStr = baseTopic + mac;
  //pubTopic = pubStr.c_str();
  pubTopic = &pubStr[0];
  pubTopic = "$aws/things/aws-arduino-iot/shadow/update/94-b9-7e-ab-d5-0";
  Serial.print("pubTopic: ");
  Serial.println(pubTopic);

  //subscribe topic
  String subStr = pubStr + "/delta";
  subTopic = &subStr[0];
  Serial.print("subTopic: ");
  Serial.println(subTopic);

  // create public key and private key object
  httpsClient.setCACert(ROOT_CA);
  httpsClient.setCertificate(CERTIFICATE);
  httpsClient.setPrivateKey(PRIVATE_KEY);

  mqttClient.setServer(MQTT_ENDPOINT, PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(MQTT_MAX_PACKET_SIZE);

  // connect to AWS IoT
  connectAWSIoT();
}

/*
 * setup wifi api endpoint hundler
 */
void handleSetupWifiApi()
{
  JSONVar obj;
  String message;
  int errorCode;
  String res;

  if (server.method() == HTTP_GET)
  {

    // WiFi.scanNetworks で検出されたssidの個数を返却する
    int n = WiFi.scanNetworks();
    Serial.println(n);

    JSONVar wifi;
    for (int i = 0; i < n; i++)
    {
      wifi["rssi"] = WiFi.RSSI(i);
      wifi["ssid"] = (String)WiFi.SSID(i);
      obj["accessPoint"][i] = wifi;
      delay(10);
    }

    obj["message"] = "ok";
    obj["errorCode"] = SUCCESSED;
    res = JSON.stringify(obj);

    // Access Log
    Serial.println("[http] [GET] [" + url_path + "]  " + res);
  }
  else if (server.method() == HTTP_POST)
  {

    obj = JSON.parse(server.arg("plain"));
    const char *id = obj["ssid"];
    const char *pw = obj["password"];

    // try connection
    M5.dis.drawpix(0, green);

    if (tryConnection(id, pw))
    {
      message = "SUCCESSED";
      errorCode = SUCCESSED;
      M5.dis.drawpix(0, blue);
    }
    else
    {
      message = "FAILED";
      errorCode = FAILED;
      M5.dis.drawpix(0, red);
    }

    obj["message"] = message;
    obj["errorCode"] = errorCode;
    res = JSON.stringify(obj);

    // Access Log
    Serial.println("[http] [POST] [" + url_path + "]  " + res);
  }

  // Response
  server.send(200, "application/json", res);
}

/*
 * api endpoint not found handler
 */
void handleNotFound()
{
  String message = "{\"message\": \"not found\", \"errorCode\": -1}";
  server.send(404, "application/json", message);
}

/*
 * default mode setting
 */
boolean modeDefault()
{
  Serial.println("Boot mode: " + modeStr[DEFAULT_MODE]);

  // Read Wifi Setting file as String
  String configStr = readFile(wifi_settings);

  // read config and split str
  String wifi[2] = {"\0"};
  split(configStr, '\n', wifi);

  String ssid = wifi[0];
  String password = wifi[1];

  ssid.trim();
  password.trim();

  Serial.println("SSID: " + ssid);
  Serial.println("PASS: " + password);

  if (ssid == null && password == null)
  {
    Serial.println("initial start");
  }
  else
  {

    const char *id = (const char *)ssid.c_str();
    const char *pw = (const char *)password.c_str();

    // check wifi setting
    boolean wlConnected = tryConnection(id, pw);

    if (wlConnected)
    {

      Serial.print("Connected to ");
      Serial.println(id);
      Serial.print("Local IP address: ");
      Serial.println(WiFi.localIP());

      // change wifi mode
      WiFi.disconnect();
      WiFi.mode(WIFI_STA);
      WiFi.begin(id, pw);

      uint8_t mac0[6];
      WiFi.macAddress(mac0);
      int size = 6;

      delay(1000);

      // Sync NTP Client
      syncNTPClient();
      delay(1000);

      // mqtt setup
      initMqtt(mac0, size);

      Serial.println("");
      Serial.println("Terrarium is up.");

      return true;
    }
  }
  return false;
}

/*
 * setup mode setting
 */
boolean modeSetup()
{

  M5.dis.drawpix(0, yellow);

  Serial.println("Boot mode: " + modeStr[SETUP_MODE]);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(WIFI_SSID, WIFI_PASS);

  Serial.print("SSID: ");
  Serial.println(WIFI_SSID);
  Serial.print("PASS: ");
  Serial.println(WIFI_PASS);
  Serial.println("Setup API: http://" + WiFi.softAPIP().toString() + url_path);
  Serial.println("");

  server.on(url_path, handleSetupWifiApi);
  server.onNotFound(handleNotFound);
  server.begin();

  return true;
}

/** 
 *  setup
 */
void setup()
{

  Serial.begin(SERIAL_BPS);
  delay(3000);
  Serial.println("");

  // M5 setup
  M5.begin(true, false, true);
  delay(10);
  M5.dis.setBrightness(10);

  // initialize file system
  if (!SPIFFS.begin(true))
  {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  Serial.print("ESP Mac Address: ");
  Serial.println(mac);

  // defult setup
  isReady = modeDefault();
}

/**
 * main loop
 */
int loopMode = DEFAULT_MODE;

boolean switchState = false;
boolean beforeState = true;
boolean isChangeMode = false;
boolean currentWifiState = false;
boolean beforeWifiState = false;
unsigned long pushTime;
unsigned long startTime;

void loop()
{
  currentWifiState = WiFi.status() == WL_CONNECTED;
  if (currentWifiState != beforeWifiState)
  {
    Serial.print("Wifi Status is Change");
    // status LED
    if (WiFi.status() == WL_CONNECTED)
    {
      M5.dis.drawpix(0, blue);
      beforeWifiState = currentWifiState;
    }
    else
    {
      M5.dis.drawpix(0, red);
      beforeWifiState = currentWifiState;
    }
  }

  // M5 Button read
  M5.Btn.read();

  // read the mode change switch input
  switchState = M5.Btn.isPressed();

  // check botton push status
  if (switchState && !beforeState)
  {

    startTime = millis();
    isChangeMode = true;
  }
  else if (switchState && beforeState)
  {

    // pushing time measurement
    pushTime = millis() - startTime;
  }
  else
  {
    // reset
    pushTime = 0;
  }
  beforeState = switchState;

  // change modo (SETUP⇄DEFAULT)
  if (pushTime > BUTTON_PUSH_TERM && isChangeMode)
  {
    isChangeMode = false;

    if (loopMode == DEFAULT_MODE)
    {

      loopMode = SETUP_MODE;
      isReady = modeSetup();
    }
    else if (loopMode == SETUP_MODE)
    {

      loopMode = DEFAULT_MODE;
      isReady = modeDefault();
    }
  }

  if (isReady)
  {
    if (loopMode == SETUP_MODE)
    {
      // boot as API server
      server.handleClient();
    }
    else if (loopMode == DEFAULT_MODE)
    {
      // boot as mqtt Client
      mqttLoop();
    }
  }
}
