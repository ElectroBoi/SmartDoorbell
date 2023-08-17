#include "BluetoothSerial.h"
#include <String>
#include "esp_camera.h"
#include <WiFi.h>
#include <EEPROM.h>
#include <ArduinoJson.h>


#define EEPROM_SIZE 512


#define CAMERA_MODEL_AI_THINKER // Has PSRAM
#include "camera_pins.h"

//#define USE_PIN // Uncomment this to use PIN during pairing. The pin is specified on the line below
const char *pin = "1234"; // Change this to more secure PIN.

String device_name = "SmartDoorbell";

BluetoothSerial SerialBT;
void EEPROM_put(char add, String data);
String EEPROM_get(char add);

int metaAddress = 0;
int metaLenght = 4;
int jsonAddress = 4;

String value = "";
String WifiPassword = "";
String WifiSSID = "";

int WifiTimout = 50;


void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  DynamicJsonDocument jsonDoc(EEPROM_SIZE);
  setEEPROM_JSON(jsonDoc);

}

void loop() {
  DynamicJsonDocument jsonDoc = getEEPROM_JSON();


  //FOR DEBUGGING ONLY
  if (Serial.available()) {
    String SerialTemp = Serial.readString();
    if (SerialTemp = "WifiStat"){
      Serial.println("SSID = " + WifiSSID);
      Serial.println("Password = " + WifiPassword);
    }
  }

  //try to connect to Wifi, when The SSID and Password is not blank
  if (jsonDoc["WifiPassword"].as<String>() != "" && jsonDoc["WifiSSID"].as<String>() != "") {
    if(jsonDoc["WifiPassword"] == "none"){
      WifiPassword = "";
    }else {
      WifiPassword = jsonDoc["WifiPassword"].as<String>();
    }
  WifiSSID = jsonDoc["WifiSSID"].as<String>();
    btStop();
    WiFi.begin(WifiSSID, WifiPassword);
    WiFi.setSleep(false);

    //waiting loop to connect WiFi
    for (int c = 0; WiFi.status() != WL_CONNECTED && c <= WifiTimout; c++) {
      delay(500);
      Serial.print(".");
      Serial.println(c);
    }
    Serial.println("");
  }
  if (WiFi.status() == WL_CONNECTED){
    Serial.println("");
    Serial.println("WiFi connected");

  }else if (WiFi.status() != WL_CONNECTED){
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF); 
    jsonDoc["WifiSSID"] = "";
    jsonDoc["WifiPassword"] = "";
    setEEPROM_JSON(jsonDoc);
    Serial.println("Wifi Connection Error");
    BTConnect();
    BTRequestData(401);
  }

  WiFi.disconnect(true);
  

  delay(20);
}



String EEPROM_read(int index, int length) {
  String text = "";
  char ch = 1;

  for (int i = index; (i < (index + length)) && ch; ++i) {
    if (ch = EEPROM.read(i)) {
      text.concat(ch);
    }
  }
  return text;
}

int EEPROM_write(int index, String text) {
  for (int i = index; i < text.length() + index; ++i) {
    EEPROM.write(i, text[i - index]);
  }
  EEPROM.write(index + text.length(), 0);
  EEPROM.commit();

  return text.length() + 1;
}

DynamicJsonDocument getEEPROM_JSON() {

  String jsonRead = EEPROM_read(jsonAddress,
                                EEPROM_read(metaAddress, metaLenght).toInt());

  // Serial.print("JSON Read: ");
  // Serial.println(jsonRead);

  DynamicJsonDocument jsonDoc(EEPROM_SIZE);

  deserializeJson(jsonDoc, jsonRead);

  return jsonDoc;
}

void setEEPROM_JSON(DynamicJsonDocument jsonDoc) {

  String jsonWriteString;

  serializeJson(jsonDoc, jsonWriteString);

  // Serial.print("JSON Write: ");
  // Serial.println(jsonWriteString);

  EEPROM_write(metaAddress,(String)EEPROM_write(jsonAddress, jsonWriteString));
}


void BTConnect(){
  SerialBT.begin(device_name);
  Serial.printf("The device with name \"%s\" is started.\nNow you can pair it with Bluetooth!\n", device_name.c_str());
  #ifdef USE_PIN
  SerialBT.setPin(pin);
  Serial.println("Using PIN");
  #endif
}

void BTRequestData(int errCode){
  DynamicJsonDocument jsonDoc = getEEPROM_JSON();
  while(jsonDoc["WifiPassword"].as<String>() == "" && jsonDoc["WifiSSID"].as<String>() == ""){

    if(SerialBT.available()){ 
      value = SerialBT.readString();
      switch(value.substring(0,2).toInt()){
        case (10):
          value.remove(0,2);
          jsonDoc["WifiPassword"] = value.c_str();
          Serial.println("Pass Register Successfull");
          break;
        case (20):
          value.remove(0,2);
          jsonDoc["WifiSSID"] = value.c_str();
          Serial.println("UUID Register Successfull");
          break;
      }
      setEEPROM_JSON(jsonDoc);
    }
    delay(20);
  }
}