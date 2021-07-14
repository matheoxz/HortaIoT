//Arduino Library
#include <Arduino.h>
//Serial communication library
#include <SoftwareSerial.h>
// library to encapsulate data into JSON format
#include <ArduinoJson.h>

// Using pin 5 as Rx and pin 6 as Tx
SoftwareSerial s(5,6);

void setup() {
  Serial.begin(9600);
  // setup of Serial communication 
  s.begin(9600);
}

void loop() {
  //creating the JSON document
  StaticJsonDocument<500> doc;
  doc["timestamp"]=millis();

  // reading each sensor and storing the values into the JSON document
    //implement
  serializeJsonPretty(doc, Serial);

  //send the JSON document to the NodeMCU
  if(s.available()>0) serializeJson(doc, s);

  delay(5000);
}
