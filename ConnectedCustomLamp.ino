
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h> //  https://github.com/kakopappa/sinric/wiki/How-to-add-dependency-libraries
#include <ArduinoJson.h> // https://github.com/kakopappa/sinric/wiki/How-to-add-dependency-libraries (use the correct version)
#include <StreamString.h>

#include <FastLED.h>
#include "Config.h"


ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;
WiFiClient client;

#define HEARTBEAT_INTERVAL 300000 // 5 Minutes 

uint64_t heartbeatTimestamp = 0;
bool isConnected = false;

int leds_number = 13;
CRGB leds[13];

boolean Multicolor = false;
int MulticolorColor = 0;

void SetColor(String color){
  if(color == "red"){
    for(int i=0;i<leds_number;i++){
      leds[i] = CRGB ( 255, 0, 0);
    }
  }
  else if(color == "blue"){
    for(int i=0;i<leds_number;i++){
      leds[i] = CRGB ( 0, 0, 255);
    }
  }
  else if(color == "green"){
    for(int i=0;i<leds_number;i++){
      leds[i] = CRGB ( 0, 255, 0);
    }
  }
  else if(color == "orange"){
    for(int i=0;i<leds_number;i++){
      leds[i] = CRGB (255,69,0);
    }
  }
  else if(color == "skyBlue"){
    for(int i=0;i<leds_number;i++){
      leds[i] = CRGB (135,206,235);
    }
  }
  
  FastLED.show();
}

void MulticolorChange(){
  
  if(MulticolorColor == 0){
    SetColor("red");
  }
  else if(MulticolorColor == 1){
    SetColor("orange");
  }
  else if(MulticolorColor == 2){
    SetColor("blue");
  }else{
    SetColor("green");
    MulticolorColor = -1;
  }

  MulticolorColor++;
  delay(5000);
}

void SwitchOffLights(){
  for(int i=0;i<leds_number;i++){
    leds[i] = CRGB ( 0, 0, 0);
  }
  FastLED.show();
}

// deviceId is the ID assgined to your smart-home-device in sinric.com dashboard. Copy it from dashboard and paste it on the config file

void turnOn(String deviceId) {
  if (deviceId == FirstDeviceID)
  {  
    Serial.print("Turn on device Esp: ");
    Serial.println(deviceId);
  } 
  else if (deviceId == SecondDeviceID || (Multicolor))
  {
    Multicolor = true;
    MulticolorColor = 0;
  }
  else {
    Serial.print("Turn on for unknown device id: ");
    Serial.println(deviceId);    
  }     
}

void turnOff(String deviceId) {
   if (deviceId == FirstDeviceID)
   {  
     Serial.print("Turn off Device Esp: ");
     Serial.println(deviceId);
     Multicolor = false; //Pour prevenir l'usage du mauvais nom
     SwitchOffLights();
   }
   else if (deviceId == SecondDeviceID)
   {
     Multicolor = false;
     Serial.print("Turn off Device ID: ");
     Serial.println(deviceId);
     SwitchOffLights();
    
  }
  else {
     Serial.print("Turn off for unknown device id: ");
     Serial.println(deviceId);    
  }
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      isConnected = false;    
      Serial.printf("[WSc] Webservice disconnected from sinric.com!\n");
      break;
    case WStype_CONNECTED: {
      isConnected = true;
      Serial.printf("[WSc] Service connected to sinric.com at url: %s\n", payload);
      Serial.printf("Waiting for commands from sinric.com ...\n");        
      }
      break;
    case WStype_TEXT: {
        Serial.printf("[WSc] get text: %s\n", payload);
        // Example payloads

        // For Switch or Light device types
        // {"deviceId": xxxx, "action": "setPowerState", value: "ON"} // https://developer.amazon.com/docs/device-apis/alexa-powercontroller.html

        // For Light device type
        // Look at the light example in github
          
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject((char*)payload); 
        String deviceId = json ["deviceId"];     
        String action = json ["action"];


        if(action == "setPowerState" || (action == "action.devices.commands.OnOff")) { // Switch or Light
            String value = json ["value"];
            if(value == "ON" || (value)) { //First test for Amazon Alexa second for Google Assistant
                turnOn(deviceId);
            } else {
                turnOff(deviceId);
            }
        }
        else if (action == "SetTargetTemperature") {
            String deviceId = json ["deviceId"];     
            String action = json ["action"];
            String value = json ["value"];
        }
        else if (action == "test") {
            Serial.println("[WSc] received test command from sinric.com");
        }
        else if((action == "SetColor") || (action == "action.devices.commands.ColorAbsolute")){
          String color;
          if(action == "SetColor"){
            int hue = json["value"]["hue"];
            
            if(hue == 0){ color = "red"; }
            else if(hue == 39){ color = "orange"; }
            else if(hue == 120){ color = "green"; }
            else if(hue == 197){ color = "skyBlue"; }
            else if(hue == 240){ color = "blue"; }     
            else if(hue == 300){ color = "violet"; }

            Serial.println("hue: "+ String(hue));
          }else{
            String couleur = json["value"]["color"]["name"];
            
            if(couleur == "rouge"){
              color = "red";
            }
            else if(couleur == "bleu"){
              color = "blue";
            }
            else if(couleur == "orange"){
              color = "orange";
            }
            else if((couleur == "verre") || (couleur = "vert")){ //vert est écrit de cette manière dans le code de Google
              // Ajout d'un patch au cas où ils fixeraient le bug
              color = "green";
            }
            
          }
            
            
            
            
            Serial.println("color: " + color);
            SetColor(color);
            
        }
      }
      break;
    case WStype_BIN:
      Serial.printf("[WSc] get binary length: %u\n", length);
      break;
  }
}

void setup() {
  pinMode( D6, OUTPUT);
  FastLED.addLeds<WS2812B, D6, GRB>(leds, leds_number);
  
  Serial.begin(115200);
  
  WiFiMulti.addAP(MySSID, MyWifiPassword);
  Serial.println();
  Serial.print("Connecting to Wifi: ");
  Serial.println(MySSID);  

  // Waiting for Wifi connect
  while(WiFiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  if(WiFiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("WiFi connected. ");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }

  // server address, port and URL
  webSocket.begin("iot.sinric.com", 80, "/");

  // event handler
  webSocket.onEvent(webSocketEvent);
  webSocket.setAuthorization("apikey", MyApiKey);
  
  // try again every 5000ms if connection has failed
  webSocket.setReconnectInterval(5000);   // If you see 'class WebSocketsClient' has no member named 'setReconnectInterval' error update arduinoWebSockets
}

void loop() {
  webSocket.loop();

  if(Multicolor){
    MulticolorChange();
  }
  
  if(isConnected) {
      uint64_t now = millis();
      
      // Send heartbeat in order to avoid disconnections during ISP resetting IPs over night. Thanks @MacSass
      if((now - heartbeatTimestamp) > HEARTBEAT_INTERVAL) {
          heartbeatTimestamp = now;
          webSocket.sendTXT("H");          
      }
  }   
}

// If you want a push button: https://github.com/kakopappa/sinric/blob/master/arduino_examples/switch_with_push_button.ino  
