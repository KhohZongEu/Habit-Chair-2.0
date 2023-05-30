#include <Arduino.h>
//-----------------Firebase------------------ 
#include "WiFi.h"
#include <Firebase_ESP_Client.h>

#define wifi_ssid "CEC"
#define wifi_password "CEC_2018"

#define API_KEY "AIzaSyAHmdwFtSJ7YgBuNHSVdR9mTFHPBYi3Ta4"
#define DATABASE_URL "https://habit-chair-default-rtdb.asia-southeast1.firebasedatabase.app"
#define USER_EMAIL "esp1@gmail.com"
#define USER_PASSWORD "123456"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

//----------------Load Cell-------------------------
#include "HX711.h"
const int FRONT_LEFT_DOUT_PIN = 19;
const int FRONT_LEFT_SCK_PIN = 18;
const int FRONT_RIGHT_DOUT_PIN = 23;
const int FRONT_RIGHT_SCK_PIN = 22;
HX711 Lscale;
HX711 Rscale;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());

    /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  fbdo.setResponseSize(4096);

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  signupOK = true;
  Lscale.begin(FRONT_LEFT_DOUT_PIN, FRONT_LEFT_SCK_PIN);
  Lscale.set_scale(-56332);
  Lscale.tare();
  Rscale.begin(FRONT_RIGHT_DOUT_PIN, FRONT_RIGHT_SCK_PIN);
  Rscale.set_scale(60654);
  Rscale.tare();
}

void sendDataToFirebase(){
    if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)){
      sendDataPrevMillis = millis();
      Firebase.RTDB.setInt(&fbdo, "data/time/hours", 5);
      Firebase.RTDB.setInt(&fbdo, "data/time/minutes", 7);
      Firebase.RTDB.setInt(&fbdo, "data/time/seconds", 4);
      Firebase.RTDB.setInt(&fbdo, "data/violations/times", 34);  
      Firebase.RTDB.setInt(&fbdo, "data/online/state", 78);
      Firebase.RTDB.setInt(&fbdo, "data/occupancy/status", 1);
  }
}

void receiveDataFromFirebase(){
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    if (Firebase.RTDB.getInt(&fbdo, "data/time/hours")) {
      if (fbdo.dataType() == "int") {
        byte H = fbdo.intData();
        Serial.print("Hours: ");
        Serial.println(H);
      }
    }
    else {
      Serial.println(fbdo.errorReason());
    }
    if (Firebase.RTDB.getInt(&fbdo, "data/time/minutes")) {
      if (fbdo.dataType() == "int") {
        byte M = fbdo.intData();
        Serial.print("Minutes: ");
        Serial.println(M);
      }
    }
    else {
      Serial.println(fbdo.errorReason());
    }
    if (Firebase.RTDB.getInt(&fbdo, "data/time/seconds")) {
      if (fbdo.dataType() == "int") {
        byte S = fbdo.intData();
        Serial.print("Seconds: ");
        Serial.println(S);
      }
    }
    else {
      Serial.println(fbdo.errorReason());
    }
    if (Firebase.RTDB.getInt(&fbdo, "data/violations/times")) {
      if (fbdo.dataType() == "int") {
        byte T = fbdo.intData();
        Serial.print("Violations: ");
        Serial.println(T);
      }
    }
    else {
      Serial.println(fbdo.errorReason());
    }
    if (Firebase.RTDB.getInt(&fbdo, "data/online/state")) {
      if (fbdo.dataType() == "int") {
        byte St = fbdo.intData();
        Serial.print("Online: ");
        Serial.println(St);
      }
    }
    else {
      Serial.println(fbdo.errorReason());
    }
    if (Firebase.RTDB.getInt(&fbdo, "data/occupancy/status")) {
      if (fbdo.dataType() == "int") {
        byte Sts = fbdo.intData();
        Serial.print("Occupancy: ");
        Serial.println(Sts);
      }
    }
    else {
      Serial.println(fbdo.errorReason());
    }
}
}

void getLoadCellReading(){
  Serial.print("Left: ");
  Serial.println(Lscale.get_value(5));
  Serial.print("right: ");
  Serial.println(Rscale.get_value(5));
}

void processLoadCellReading(){

}



void audioAlert(){

}

void loop() {
  //sendDataToFirebase();
  //receiveDataFromFirebase();
  getLoadCellReading();
}