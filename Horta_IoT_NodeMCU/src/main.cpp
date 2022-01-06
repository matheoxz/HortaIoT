/*************************INCLUDES*************************/
#include <Arduino.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <MQTT.h>
#include <string.h>
#include <SoftwareSerial.h>
/*********************************************************/

/*********************GLOBAL VARIABLES********************/
//config files
StaticJsonDocument<400> doc;
JsonObject obj;

//Wifi configuration
char wifiSsid[20];
char wifiPassword[30];

//MQTT broker and credentials configuration
char brokerUrl[256];
char brokerUsername[20];
char brokerPassword[20];

//PubSub client
WiFiClientSecure net;
MQTTClient client;

//Serial communication, (Rx,Tx)
SoftwareSerial s(D6, D5);
unsigned long lastMsg = 0, lastMsg10 = 0;
StaticJsonDocument<500> msg;
char data[500];

/*********************************************************/

/************************FUNCTIONS************************/

// load the configuration file data
bool loadConfig()
{
  //saveConfig();
  //opens the configuration file in read mode
  File configFile = LittleFS.open("/secrets.json", "r");

  //check the existence of the configuration file
  if (!configFile)
  {
    Serial.println("failed to open configuration file");
    return false;
  }
  //allocate a buffer to store the contents of the file and
  //after use it with ArduinoJson deserialize function
  size_t size = configFile.size();
  std::unique_ptr<char[]> buf(new char[size]);

  //Read the file in Json format
  configFile.readBytes(buf.get(), size);

  auto error = deserializeJson(doc, buf.get());
  if (error)
  {
    Serial.println("Failed to parse configuration file");
    return false;
  }

  obj = doc.as<JsonObject>();

  //assign the results to the global variables

  strcpy(brokerUsername, obj["broker_username"]);
  strcpy(brokerPassword, obj["broker_password"]);

  strcpy(wifiSsid, obj["wifi_ssid"]);
  strcpy(wifiPassword, obj["wifi_password"]);

  strcpy(brokerUrl, obj["broker_url"]);

  return true;
}

// setup the WiFi and connect to it
void setupWiFi()
{
  Serial.print("Connecting to ");
  Serial.println(wifiSsid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSsid, wifiPassword);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

//Reconnect to broker
void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "Horta-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    net.setInsecure();

    if (client.connect(clientId.c_str(), brokerUsername, brokerPassword))
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("led");
      client.subscribe("motor");
    }
    else
    {

      Serial.print("failed, rc=");
      Serial.print(client.lastError());

      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void messageReceived(String &topic, String &payload)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(payload);

  //control illumination
  if(topic.compareTo("led") == 0){
    Serial.print("led: ");
    //First floor control
    if((char)payload[0]=='0'){
      digitalWrite(D4, LOW);
      Serial.println("1, off ");
    }
    if((char)payload[0]=='1'){
      digitalWrite(D4, HIGH);
      Serial.println("1, on ");
    }

    //second floor control
    if((char)payload[0]=='2'){
      digitalWrite(D3, LOW);
      Serial.println("2, off ");
    }
    if((char)payload[0]=='3'){
      digitalWrite(D3, HIGH);
      Serial.println("2, on ");
    }
  }

  //control motor
  if(topic.compareTo("motor")==0){
    Serial.print("motor: ");
    if((char)payload[0]=='0'){
      digitalWrite(D2, LOW);
      Serial.println("off ");
    }
    if((char)payload[0]=='1'){
      digitalWrite(D2, HIGH);
      Serial.println("on ");
    }

  }

}

void deserializeData(){
  if (s.available())
  {
    DeserializationError err = deserializeJson(msg, s);
    serializeJson(msg, data);
    client.publish("data", data);
    if (err == DeserializationError::Ok)
    {
      client.publish("success", "deserialization ok");
    }
    else
    {
      client.publish("error", err.c_str());

      // Flush all bytes in the "link" serial port buffer
      while (s.available() > 0)
        s.read();
    }
  }
}

/*********************************************************/

/***************************MAIN**************************/
void setup()
{
  //start serial monitor
  Serial.begin(115200);

  //start serial communication to Arduino board;
  s.begin(9600);

  //mount the file system to access the configuration file
  Serial.println("Mounting file system...");
  if (!LittleFS.begin())
  {
    Serial.println("failed to mount file system");
  }

  //load configuration file
  if (!loadConfig())
  {
    Serial.println("Failed to load configuration file");
  }
  else
  {
    Serial.println("Configuration loaded");
  }

  //connect to WiFi
  setupWiFi();

  //set the pinouts
  pinMode(D4, OUTPUT); //first floor illumination
  pinMode(D3, OUTPUT); //second floor illumination
  pinMode(D2, OUTPUT); //motor

  //connect to MQTT broker
  client.begin(brokerUrl, 8883, net);
  //set callback funtion to when it receives a message
  client.onMessage(messageReceived);

  delay(500);
}

void loop()
{

  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();

  if (now - lastMsg > 5000)
  {
    lastMsg = now;
    deserializeData();
  }


  if (now - lastMsg10 > 10000)
  {
    lastMsg10 = now;

    //Serial.println("Publish message: debug");

    client.publish("debug", "debug");
  }
}

/*********************************************************/
