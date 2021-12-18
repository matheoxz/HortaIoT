/********************************Includes**************************************/
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

/*********************************Defines**************************************/
#define PH_PORT A0
#define EC_PORT A1
#define TEMPERATURE_PORT 3
#define ILLUMINANCE_PORT A2
#define LEVEL_PORT_1 9
#define LEVEL_PORT_2 10
#define LEVEL_PORT_3 11

/****************************Global Variables**********************************/
// Using pin 5 as Rx and pin 6 as Tx
SoftwareSerial nodemcu(5,6);

//temperature sensor variables
OneWire oneWire(TEMPERATURE_PORT);
DallasTemperature sensors(&oneWire);

//GravityTDS
GravityTDS gravityTDS;

/**********************************Setup***************************************/
void setup() {
  Serial.begin(9600);
  // setup of Serial communication
  nodemcu.begin(9600);
  delay(1000);

  //temperature
  oneWire.write(0x44, 1); //set parasiteMode
  sensors.begin(); //start temperature

  //pH
  pinMode(PH_PORT, INPUT);

  //ec
  gravityTDS.setPin(EC_PORT);
  gravityTDS.setAref(5.0);
  gravityTDS.setAdcRange(1024);
  gravityTDS.begin();

  //lux port
  pinMode(ILLUMINANCE_PORT, INPUT);

  //level ports
  pinMode(LEVEL_PORT_1, INPUT);
  pinMode(LEVEL_PORT_2, INPUT);
  pinMode(LEVEL_PORT_3, INPUT);
}
/****************************Reading Functions*********************************/
//Read pH value
double getPh(){
  //reading analog port
  int adcValue;
  for(int i = 0; i < 20; i++){
    adcValue += analogRead(PH_PORT);
    delay(100);
  }
  adcValue /= 20;
  //converting byte to voltage
  double phVoltage = (float)adcValue * 5.0 / 1024.0;

  //calculate pH -> phMax - (VphMax - phVoltage) * m
  return 6.86 - (2.40 - phVoltage) * (-5.6078431373) ;
}

float getEc(float temperature){
  gravityTDS.setTemperature(temperature);  // set the temperature and execute temperature compensation
  gravityTDS.update();  //sample and calculate
  float tdsValue = gravityTDS.getEcValue();
  return tdsValue;
}

float getTemperature(){
  //request temperature from sensor
  sensors.requestTemperatures();
  //convert to Celsius
  return sensors.getTempCByIndex(0);
}

float getIlluminance(){
  float mean = 0;
  for(int i = 0; i < 20; i++){
  mean += analogRead(ILLUMINANCE_PORT);
  }
  return map(mean/20, 0, 1024, 1, 100);
}

int getLevel(){
  //reading digital ports
  int l1 = digitalRead(LEVEL_PORT_1);
  int l2 = digitalRead(LEVEL_PORT_2);
  int l3 = digitalRead(LEVEL_PORT_3);

  //checking level
  if(l1 && l2 && l3) return 100;
  else if(l1 && l2 && !l3) return 50;
  else if(l1 && !l2 && !l3) return 0;
  return -1;
}
/**********************************Loop****************************************/
void loop() {
  //creating the JSON document
  StaticJsonDocument<500> doc;
  // reading each sensor and storing the values into the JSON document
  doc["temperature"] = getTemperature();
  doc["pH"] = getPh();
  doc["ec"] = getEc(doc["temperature"]);
  doc["illuminance"] = getIlluminance();
  doc["level"] = getLevel();
  //serialize JSON with read data
  serializeJsonPretty(doc, Serial);

  //send the JSON document to the NodeMCU
  if(nodemcu.available()>0) serializeJson(doc, nodemcu);

  delay(500);
}
/******************************************************************************/
