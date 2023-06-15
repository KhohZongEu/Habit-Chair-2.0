#include <Arduino.h>

#define Running 2
#define runInterval 500
unsigned long previousMillis;
bool ledState;
bool occupancy = 1;
long Hrecord = 0;
long Mrecord = 0;
long Srecord = 0;
byte AHT;
byte AMT;
byte AST;
//-------------NTP server details--------------------
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 28800;//timezone change
const int daylightOffset_sec = 0; // Set daylight offset difference
byte timeDigits[2]={0,0};
//------------------------------DF Player Mini------------------------------
#include <DFPlayerMini_Fast.h>
#include <SoftwareSerial.h>  //file 0000 is bad posture, 0001 is sitting time, 0002 is left leg, 0003 is right leg
SoftwareSerial mySerial(25,26);//Tx,Rx
DFPlayerMini_Fast myMP3;
uint8_t volume = 30; // max = 30
bool audioPlaying;
//-----------------Firebase------------------ 
#include "WiFi.h"
#include <Firebase_ESP_Client.h>

#define wifi_ssid "Nemo5"
#define wifi_password "Nem0123456789"

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
#define FRONT_LEFT_DOUT_PIN 19 //Blue
#define FRONT_LEFT_SCK_PIN 18 //Green
#define FRONT_RIGHT_DOUT_PIN 23 //Orange
#define FRONT_RIGHT_SCK_PIN 22 //Yellow
#define BACK_LEFT_DOUT_PIN 16 // White RX
#define BACK_LEFT_SCK_PIN 17 // Brown TX
#define BACK_RIGHT_DOUT_PIN 32 //Grey
#define BACK_RIGHT_SCK_PIN 33 //Purple

HX711 BackLscale;
HX711 BackRscale;
HX711 FrontLscale;
HX711 FrontRscale;

int backRightScale = 17;
int backLeftScale = 16;
int frontRightScale = 13;
int frontLeftScale = 12;

byte postureState = 0;

void FirebaseInit(){
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
}

void WifiConnect(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  FirebaseInit();
}

void setup() {
  Serial.begin(115200);
  pinMode(Running,OUTPUT);
  //configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  //WifiConnect();
  
  FrontLscale.begin(FRONT_LEFT_DOUT_PIN, FRONT_LEFT_SCK_PIN);
  FrontLscale.set_scale(-54824.7012);
  FrontLscale.tare();
  FrontRscale.begin(FRONT_RIGHT_DOUT_PIN, FRONT_RIGHT_SCK_PIN);
  FrontRscale.set_scale(-60794.6821);
  FrontRscale.tare();
  BackLscale.begin(BACK_LEFT_DOUT_PIN,BACK_LEFT_SCK_PIN);
  BackLscale.set_scale(57595.9616);
  BackLscale.tare();
  BackRscale.begin(BACK_RIGHT_DOUT_PIN,BACK_RIGHT_SCK_PIN);
  BackRscale.set_scale(56341.4634);
  BackRscale.tare();
}

void getLocalTime(){
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("[ERROR]: Failed to obtain time");
    return;
  }
  timeDigits[0] = timeinfo.tm_hour;
  timeDigits[1] = timeinfo.tm_min;
  Serial.println("Actual time: ");
  Serial.print(timeDigits[0]);
  Serial.println(timeDigits[1]);
}

void calculateTime(){
  unsigned long timeCheck = 0;
  unsigned long TimeElapsed = millis() - timeCheck;
  if(occupancy ==1){
    if(TimeElapsed >= 1000){
      timeCheck = millis();
      Srecord = Srecord + 1;
      Serial.println("Time Change: ");
      Serial.print("Second: ");
      Serial.println(Srecord);
      Serial.print("Minute: ");
      Serial.println(Mrecord);
      if(Srecord > 60){
        Srecord = 0;
        Mrecord = Mrecord +1;
        if(Mrecord >60){
          Mrecord = 0;
          Hrecord = Hrecord +1;
        }
      } 
    }
  }
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
  backLeftScale = BackLscale.get_units(10);
  Serial.print("Back Left: ");
  Serial.println(BackLscale.get_units(10));
  backRightScale = BackRscale.get_units(10);
  Serial.print("Back Right: ");
  Serial.println(BackRscale.get_units(10));
  frontRightScale = FrontRscale.get_units(10);
  Serial.print("Front Left: ");
  Serial.println(FrontLscale.get_units(10));
  frontLeftScale = FrontRscale.get_units(10);
  Serial.print("Front Right: ");
  Serial.println(FrontRscale.get_units(10));
}

void processLoadCellReading(){
  if((abs((frontLeftScale+frontRightScale)-(backLeftScale+backRightScale))/2<=3) && (abs((frontLeftScale+backLeftScale)-(frontRightScale+backRightScale))/2<2)){
    Serial.println("Posture is correct");
    postureState = 1;
    occupancy = 1;
  }else if(abs((backLeftScale+backRightScale)-(frontLeftScale+frontRightScale))/2 > 3){
    Serial.println("User is Leaning Back");
    postureState = 2;
    occupancy = 1;
  }else if(abs((frontLeftScale+backLeftScale)-(frontRightScale+backRightScale))/2 > 2){
    Serial.println("User is Leaning Left");
    postureState = 3;
    occupancy = 1;
  }else if(abs((frontRightScale+backRightScale)-(frontLeftScale+backLeftScale))/2 > 2){
    Serial.println("User is Leaning Right");
    postureState = 4;
    occupancy = 1;
  }else if((frontRightScale+frontLeftScale+backLeftScale+backRightScale)/4 < 5){
    Serial.println("Chair is unoccupied");
    postureState = 1;
    occupancy = 0;
  }
}

void rawLoadCellProcess(){
  Serial.println(abs((frontLeftScale+frontRightScale)-(backLeftScale+backRightScale))/2);
}

void audioAlert(){
  audioPlaying = myMP3.isPlaying();
  switch(postureState){
    case 1:
    
    break;

    case 2:
      if(audioPlaying == false){
        myMP3.volume(30);
        myMP3.play(2);
      }
    break;

    case 3:
      if(audioPlaying == false){
        myMP3.volume(30);
        myMP3.play(3);
      }
    break;

    case 4:
      if(audioPlaying == false){
        myMP3.volume(30);
        myMP3.play(4);
      }
    break;
  }
}

void ledBlink(){
  if(millis()-previousMillis >= runInterval){
    previousMillis = millis();
    if(ledState==LOW){
      ledState = HIGH;
    }else{
      ledState = LOW;
    }
    digitalWrite(Running,ledState);
  }
}

void loop() {
  ledBlink();
  //getLocalTime();
  //calculateTime();
  //rawLoadCellProcess();
  sendDataToFirebase();
  receiveDataFromFirebase();
  getLoadCellReading();
  processLoadCellReading();
  audioAlert();
}