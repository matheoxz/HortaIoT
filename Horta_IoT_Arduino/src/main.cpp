//Arduino Library
#include <Arduino.h>
//Serial communication library
#include <SoftwareSerial.h>
// library to encapsulate data into JSON format
#include <ArduinoJson.h>

// Using pin 5 as Rx and pin 6 as Tx
SoftwareSerial nodemcu(5,6);

int teste = 0;

void setup() {
  Serial.begin(9600);
  // setup of Serial communication 
  nodemcu.begin(9600);
  delay(1000);
}

void loop() {
  //creating the JSON document
  StaticJsonDocument<500> doc;
  

  doc["timestamp"]=millis();
  doc["teste"]=teste;
  doc["aa"]="aaa";
  teste++;
  // reading each sensor and storing the values into the JSON document
    //implement
  serializeJsonPretty(doc, Serial);

  //send the JSON document to the NodeMCU
  
  if(nodemcu.available()>0) serializeJson(doc, nodemcu);

  delay(5000);
}
