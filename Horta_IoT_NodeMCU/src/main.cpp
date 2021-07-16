/*************************INCLUDES*************************/
#include <Arduino.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <MQTT.h>
#include <string.h>
/*********************************************************/


/*********************GLOBAL VARIABLES*********************/
StaticJsonDocument<400> doc;
JsonObject obj;
//Wifi configuration
char wifiSsid[20];
char wifiPassword[30];

//MQTT broker and credentials configuration
char brokerUrl[256];
char *brokerUsername;
char *brokerPassword;

//PubSub client
WiFiClientSecure net;
MQTTClient client;


//millis variable
unsigned long lastMsg = 0;

/*********************************************************/


/************************FUNCTIONS************************/

// load the configuration file data
bool loadConfig(){
  //saveConfig();
  //opens the configuration file in read mode
  File configFile = LittleFS.open("/secrets.json", "r");

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
  
  auto error = deserializeJson(doc, buf.get());
  if(error){
    Serial.println("Failed to parse configuration file");
    return false;
  }

  obj = doc.as<JsonObject>();

  //assign the results to the global variables
  brokerUsername = (char*)malloc(sizeof(obj["broker_username"]));
  strcpy(brokerUsername,obj["broker_username"]);

  brokerPassword = (char*)malloc(sizeof(obj["broker_password"]));
  strcpy(brokerPassword,obj["broker_password"]);

  
  strcpy(wifiSsid,obj["wifi_ssid"]);
  strcpy(wifiPassword,obj["wifi_password"]);

  strcpy(brokerUrl,obj["broker_url"]);

  return true;
}

// setup the WiFi and connect to it
void setupWiFi(){
  Serial.print("Connecting to ");
  Serial.println(wifiSsid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSsid, wifiPassword);

  while (WiFi.status() != WL_CONNECTED) {
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
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "Horta-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    net.setInsecure();
   
    if (client.connect(clientId.c_str(), brokerUsername, brokerPassword)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("led");
    } else {
      
      Serial.print("failed, rc=");
      Serial.print(client.lastError());
     

      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void messageReceived(String &topic, String &payload) {
   Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.print(payload);

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(LED_BUILTIN, LOW);   
  } else {
    digitalWrite(LED_BUILTIN, HIGH);  
  }
}
/*********************************************************/

/***************************MAIN**************************/
void setup() {
  //start serial monitor
  Serial.begin(115200);

  //mount the file system to access the configuration file
  Serial.println("Mounting file system...");
  if(!LittleFS.begin()){
    Serial.println("failed to mount file system");
    
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
  client.begin(brokerUrl, 8883, net);
  
  client.onMessage(messageReceived);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();


  unsigned long now = millis();
  if (now - lastMsg > 10000) {
    lastMsg = now;
   
    Serial.print("Publish message: ");
    Serial.println("hello world");
    client.publish("outTopic", "hello world");
  }
}

/*********************************************************/