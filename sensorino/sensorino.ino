/*
Copyright 2023 Agnese Salutari.
Licensed under the Apache License, Version 2.0 (the "License"); 
you may not use this file except in compliance with the License. 
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on 
an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License
*/


#include <Arduino_MKRENV.h> // MKR ENV Shield dependency
#include <TinyGPS.h>  // GPS dependency
#include <ArduinoMqttClient.h>  // Mosquitto dependency
#include <WiFiNINA.h>  // WiFi dependency

#include "config.h" // Config file

#define ledPin 2  // LED
#define wifiPin 6 // WiFi
#define gpsSerial Serial1 // rx and tx pins (microUsb is Serial, or Serial0)

struct WifiConfigStruct{
  char* ssid;
  char* pwd;
};

struct MqttConfigStruct{
  char* broker;
  int port;
  char* topic;
  char* clientId;
  char* user;
  char* pwd;
};

struct SenseStruct{
  float temperature;
  float humidity;
  float pressure;
  float illuminance;
  float uva;
  float uvb;
  float uvIndex;
  float lat;
  float lon;
};

TinyGPS gps; // GPS object
WiFiClient wifi;
MqttClient mqttClient(wifi);

// From config file
const WifiConfigStruct wifiConfig = {.ssid = WIFI_SSID, .pwd = WIFI_PWD};
const MqttConfigStruct mqttConfig = {.broker = MQTT_BROKER, .port = MQTT_PORT, .topic = MQTT_TOPIC, .clientId = MQTT_CLIENT_ID, .user = MQTT_USER, .pwd = MQTT_PWD};
//
SenseStruct sensed;  // Output
 
void setup() {
  pinMode(ledPin, OUTPUT);
  
  serialInit();
  shieldInit();
  gpsInit();
  wifiInit();
  mqttInit();

  Serial.println("Setup performed successfully");
}

void loop() {
  sense();
  mqttSend();
  debugPrint();
  blink(10);
}

void serialInit(){
  Serial.begin(9600);
  while(!Serial);
}

void shieldInit(){
  if(!ENV.begin()) {
    Serial.println("Failed to initialize MKR ENV shield!");
    while(1);
  }
  Serial.println("MKR ENV shield initialized");
}

void gpsInit(){
  gpsSerial.begin(9600); // connect gps sensor
  while(!gpsSerial);
  Serial.println("GPS initialized");
}

void wifiInit(){
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(wifiConfig.ssid);
  while(WiFi.begin(wifiConfig.ssid, wifiConfig.pwd) != WL_CONNECTED) {
    Serial.print(".");
    delay(5000);
  }
  Serial.println("You're connected to the network");
  Serial.println();
}

void mqttInit(){
  mqttClient.setId(mqttConfig.clientId);
  mqttClient.setUsernamePassword(mqttConfig.user, mqttConfig.pwd);
  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(mqttConfig.broker);
  if (!mqttClient.connect(mqttConfig.broker, mqttConfig.port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    while (1);
  }
  Serial.println("Connected to the MQTT broker");
  Serial.println();
}

void blink(int deltaSec){
  uint8_t i = 1;
  while(deltaSec >= 0){
    analogWrite(ledPin, i);
    delay(1000);
    analogWrite(ledPin, 0);
    delay(1000);
    deltaSec -= 2;
    i += 2;
  }
}

void sense(){
  sensed.temperature = ENV.readTemperature();
  sensed.humidity = ENV.readHumidity();
  sensed.pressure = ENV.readPressure();
  sensed.illuminance = ENV.readIlluminance();
  sensed.uva = ENV.readUVA();
  sensed.uvb = ENV.readUVB();
  sensed.uvIndex = ENV.readUVIndex(); 

  if(gpsSerial.available()) { // Check for gps data
    if(gps.encode(gpsSerial.read())) { // Encode gps data
      gps.f_get_position(&sensed.lat,&sensed.lon); // Get latitude and longitude
    }
  }
}

void mqttSend(){
  // call poll() regularly to allow the library to send MQTT keep alives which
  // avoids being disconnected by the broker
  mqttClient.poll();

  Serial.print("Sending data to topic: ");
  Serial.println(mqttConfig.topic);
  
  mqttClient.beginMessage(mqttConfig.topic);
  mqttClient.print("temperature:");
  mqttClient.print(sensed.temperature);
  mqttClient.print(";");
//  mqttClient.print("humidity:");
//  mqttClient.print(sensed.humidity);
//  mqttClient.print(";");
//  mqttClient.print("pressure:");
//  mqttClient.print(sensed.pressure);
//  mqttClient.print(";");
  mqttClient.print("illuminance:");
  mqttClient.print(sensed.illuminance);
  mqttClient.print(";");
//  mqttClient.print("uva:");
//  mqttClient.print(sensed.uva);
//  mqttClient.print(";");
//  mqttClient.print("uvb:");
//  mqttClient.print(sensed.uvb);
//  mqttClient.print(";");
//  mqttClient.print("uvIndex:");
//  mqttClient.print(sensed.uvIndex);
//  mqttClient.print(";");
  mqttClient.print("latitude:");
  mqttClient.print(sensed.lat);
  mqttClient.print(";");
  mqttClient.print("longitude:");
  mqttClient.print(sensed.lon);
  mqttClient.print(";");
  mqttClient.endMessage();
}

void debugPrint(){
  Serial.print("Temperature = ");
  Serial.print(sensed.temperature);
  Serial.println(" °C");

  Serial.print("Humidity = ");
  Serial.print(sensed.humidity);
  Serial.println(" %");

  Serial.print("Pressure = ");
  Serial.print(sensed.pressure);
  Serial.println(" kPa");

  Serial.print("Illuminance = ");
  Serial.print(sensed.illuminance);
  Serial.println(" lx");

  Serial.print("UVA = ");
  Serial.println(sensed.uva);

  Serial.print("UVB = ");
  Serial.println(sensed.uvb);

  Serial.print("UV Index = ");
  Serial.println(sensed.uvIndex);

  Serial.print("Position = ");
  Serial.print(sensed.lat);
  Serial.print(" - ");
  Serial.println(sensed.lon);

  Serial.println();
}
