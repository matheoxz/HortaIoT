//Arduino Library
#include <Arduino.h>
//temperature sensor libraries
#include <OneWire.h>
#include <DallasTemperature.h>

#define PH_PORT A0
#define TEMPERATURE_PORT 3

//temperature sensor variables
OneWire oneWire(TEMPERATURE_PORT);
DallasTemperature sensors(&oneWire);
float temperature;

//pH variables
int analogValue;
float VpH;

void setup() {
  Serial.begin(9600);
  //temperature
  oneWire.write(0x44, 1); //set parasiteMode
  sensors.begin(); //start temperature

  //pH
  pinMode(PH_PORT, INPUT);
}

float getTemperature(){
  //request temperature from sensor
  sensors.requestTemperatures();
  //convert to Celsius
  return sensors.getTempCByIndex(0);
}

void loop(){
  temperature = getTemperature();
  Serial.print(temperature);
  Serial.print(",");

  analogValue = analogRead(PH_PORT);
  VpH = (float)analogValue * 5.0 / 1024.0;
  Serial.print(VpH);
  Serial.print("\n");

  delay(500);
  }
