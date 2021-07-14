/*************************INCLUDES*************************/
#include <Arduino.h>
#include <FS.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
/*********************************************************/

/***********************GLOBAL TYPES**********************/
struct WiFiConfiguration
{
  const char* ssid;
  const char* password;
};

struct MQTTConfiguration
{
  const char* url;
  const char* port;
  const char* username;
  const char* password;

};


/*********************************************************/

/*********************GLOBAL VARIABLES*********************/
//Wifi configuration
WiFiConfiguration wifiConfig;

//MQTT broker and credentials configuration
MQTTConfiguration mqttConfig;

//PubSub client
WiFiClient hortaClient;
PubSubClient client(hortaClient);

//millis variable
unsigned long lastMsg = 0;

/*********************************************************/


/************************FUNCTIONS************************/

// load the configuration file data
bool loadConfig(){
  //opens the configuration file in read mode
  File configFile = SPIFFS.open("/secrets.json", "r");

  //check the existence of the configuration file
  if(!configFile){
    Serial.println("failed to open configuration file");
    return false;
  }
  //allocate a buffer to store the contents of the file and
  //after use it with ArduinoJson deserialize function
  size_t size = configFile.size();
  std::unique_ptr<char[]> buf(new char[size]);

  //Read the file in Json format
  configFile.readBytes(buf.get(), size);
  StaticJsonDocument<400> doc;
  auto error = deserializeJson(doc, buf.get());
  if(error){
    Serial.println("Failed to parse configuration file");
    return false;
  }

  //assign the results to the global variables
  wifiConfig.ssid = doc["wifi_settings"]["ssid"];
  wifiConfig.password = doc["wifi_settings"]["password"];

  mqttConfig.url = doc["mqtt_settings"]["broker"]["server_url"];
  mqttConfig.port = doc["mqtt_settings"]["broker"]["server_port"];

  mqttConfig.username = doc["mqtt_settings"]["credentials"]["username"];
  mqttConfig.password = doc["mqtt_settings"]["credentials"]["password"];
  return true;

}

// setup the WiFi and connect to it
void setupWiFi(){
  Serial.print("Connecting to ");
  Serial.println(wifiConfig.ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiConfig.ssid, wifiConfig.password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

//PubSub callback
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  
  }

}

//Reconnect to broker 
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "Horta-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("led");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
/*********************************************************/

/***************************MAIN**************************/
void setup() {
  //start serial monitor
  Serial.begin(115200);

  //mount the file system to access the configuration file
  Serial.println("Mounting file system...");
  if(SPIFFS.begin()){
    Serial.println("failed to mount file system");
    return;
  }

  //load configuration file
  if (!loadConfig()) {
    Serial.println("Failed to load configuration file");
  } else {
    Serial.println("Configuration loaded");
  }

  //connect to WiFi
  setupWiFi();

  //connect to MQTT broker
  client.setServer(mqttConfig.url, atoi(mqttConfig.port) );
  
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
   
    Serial.print("Publish message: ");
    Serial.println("hello world");
    client.publish("outTopic", "hello world");
  }
}

/*********************************************************/