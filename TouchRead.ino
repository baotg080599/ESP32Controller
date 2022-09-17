#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <FreeRTOS.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "FirebaseESP32.h"

#define FIREBASE_HOST "https://smartfarm-8f5f5-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "wKxF63iHXDA6zZCSvqtCzJ1z1xwy4QL57fATQftp"

FirebaseData firebasedata;
FirebaseJson json;

const char *SSID = "TUAN PHONG";
const char *PWD = "88091199";

const char *SSIDAP = "ESP32";
const char *PWDAP = "12345678";

const int relay1 = 26;
const int relay2 = 27;

String test;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

String formattedDate;
String dayStamp;
String timeStamp;

WebServer server(80);

// JSON data buffer
StaticJsonDocument<250> jsonDocument;
char buffer[250];

// env variable
float humidity;

void connectToWiFi() {
  Serial.print("Connecting to ");
  Serial.println(SSID);
  
  WiFi.begin(SSID, PWD);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    // we can even make the ESP32 to sleep
  }
 
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());
}

void create_json(char *tag, float value, char *unit) { 
  jsonDocument.clear(); 
  jsonDocument["type"] = tag;
  jsonDocument["value"] = value;
  jsonDocument["unit"] = unit;
  serializeJson(jsonDocument, buffer);
  Serial.println("Buffer:");
  Serial.println(buffer);  
}

void add_json_object(char *tag, float value, char *unit) {
  JsonObject obj = jsonDocument.createNestedObject();
  obj["type"] = tag;
  obj["value"] = value;
  obj["unit"] = unit; 
}

void handlePost() {
  if (server.hasArg("plain") == false) {
    //handle error here
  }

  String body = server.arg("plain");
  Serial.println(body);
  deserializeJson(jsonDocument, body);
  
  // Get RGB components
  String timeValue = jsonDocument["ssid"];
  Serial.println(timeValue);
  delay(30);
  // Respond to the client
  server.send(200, "application/json", "{}");
}

// setup API resources
void setup_routing() {
  server.on("/setwifi", HTTP_POST, handlePost);
  // start server
  server.begin();
}

void setup() {
  Serial.begin(115200);
  connectToWiFi();
  Serial.print("Setting AP mode");
  WiFi.softAP(SSIDAP, PWDAP);
  IPAddress IP = WiFi.softAPIP(); //mặc định là 192.168.4.1
  Serial.print("AP IP address: ");
  Serial.println(IP);
  Firebase.begin(FIREBASE_HOST,FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  timeClient.begin();
  timeClient.setTimeOffset(25200);
  timeClient.forceUpdate();
  while(!timeClient.update()) {
  timeClient.forceUpdate();
  }

  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  // Set outputs to LOW
  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH);
  setup_routing();  
}

void loop() {
  formattedDate = timeClient.getFormattedDate();
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
  Firebase.getInt(firebasedata,"/"+dayStamp+"/"+timeStamp);
  int result = firebasedata.intData();
  Serial.print(result);
  if(result == 1){
    digitalWrite(relay1, LOW);
    Firebase.setInt(firebasedata,"Status",1);
  }
  if(result == 2){
    digitalWrite(relay1, HIGH);
    Firebase.setInt(firebasedata,"Status",0);
  }
  Firebase.getInt(firebasedata,"start");
  int start = firebasedata.intData();
  if(start == 1){
    digitalWrite(relay1, LOW);
    Firebase.setInt(firebasedata,"start",0);
    Firebase.setInt(firebasedata,"Status",1);
  }
  if(start == 2){
    digitalWrite(relay1, HIGH);
    Firebase.setInt(firebasedata,"start",0);
    Firebase.setInt(firebasedata,"Status",0);
  }
  server.handleClient();
}
