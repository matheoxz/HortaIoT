//Arduino Library
#include <Arduino.h>
//Serial communication library
#include <SoftwareSerial.h>
// library to encapsulate data into JSON format
#include <ArduinoJson.h>
//temperature sensor libraries
#include <OneWire.h>
#include <DallasTemperature.h>

#include <GravityTDS.h>

#define PH_PORT A0
#define EC_PORT A1
#define TEMPERATURE_PORT 3
#define LUX_PORT A2
#define LEVEL_PORT_1 9
#define LEVEL_PORT_2 10
#define LEVEL_PORT_3 11

// Using pin 5 as Rx and pin 6 as Tx
SoftwareSerial nodemcu(5,6);

//temperature sensor variables
OneWire oneWire(TEMPERATURE_PORT);
DallasTemperature sensors(&oneWire);

//GravityTDS
GravityTDS gravityTDS;

void setup() {
  Serial.begin(9600);
  // setup of Serial communication
  nodemcu.begin(9600);
  delay(1000);

  //pH
  pinMode(PH_PORT, INPUT);

  //ec
  gravityTDS.setPin(EC_PORT);
  gravityTDS.setAref(5.0);
  gravityTDS.setAdcRange(1024);
  gravityTDS.begin();

  //lux port
  pinMode(LUX_PORT, INPUT);

  //level ports
  pinMode(LEVEL_PORT_1, INPUT);
  pinMode(LEVEL_PORT_2, INPUT);
  pinMode(LEVEL_PORT_3, INPUT);


  //temperature
  oneWire.write(0x44, 1); //set parasiteMode
  sensors.begin(); //start temperature

}

float getPh(){
  int adcValue = analogRead(PH_PORT);
  float phVoltage = (float)adcValue * 5.0 / 1024.0;
  //calculate m -> (phMax - phMin) / (VphMax - VphMin)
  //calculate pH -> phMax - (VphMax - phVoltage) * m
  //suposing phMax = 7, its voltage is 2.5 and a m of -5.436
  float m = -5.436;
  return 7.0 - (2.5 - phVoltage) * m ;
}

float getEc(float temperature){
  //float adcValue = analogRead(PH_PORT);
  //float rawEc = (float)adcValue * 5.0 / 1024.0;
  //float temperatureCoefficient = 1.0 + 0.2 * (temperature - 25.0);
  //float ec = (rawEc / temperatureCoefficient) * 2;
  gravityTDS.setTemperature(temperature);  // set the temperature and execute temperature compensation
  gravityTDS.update();  //sample and calculate
  float tdsValue = gravityTDS.getTdsValue();
  return tdsValue;
}

float getTemperature(){
  //request temperature from sensor
  sensors.requestTemperatures();
  //convert to Celsius
  return sensors.getTempCByIndex(0);
}

float getIlluminance(){
  float adcValue = analogRead(LUX_PORT);
  float Vout = (float)adcValue*5.0/1024.0;
  float resistance = (1000*(5-Vout))/Vout;
  int lux = 500 / (resistance/1000);
  return lux;
}

int getLevel(){
  int l1 = digitalRead(LEVEL_PORT_1);
  int l2 = digitalRead(LEVEL_PORT_2);
  int l3 = digitalRead(LEVEL_PORT_3);

  if(l1 && l2 && l3) return 100;
  else if(l1 && l2 && !l3) return 50;
  else if(l1 && !l2 && !l3) return 0;
  return -1;
}

void loop() {
  //creating the JSON document
  StaticJsonDocument<500> doc;
  // reading each sensor and storing the values into the JSON document
  doc["temperature"] = getTemperature();
  doc["pH"] = getPh();
  doc["tds"] = getEc(doc["temperature"]);
  doc["illuminance"] = getIlluminance();
  doc["level"] = getLevel();
  //serialize JSON with read data
  serializeJsonPretty(doc, Serial);

  //send the JSON document to the NodeMCU
  if(nodemcu.available()>0) serializeJson(doc, nodemcu);

  delay(1000);
}
