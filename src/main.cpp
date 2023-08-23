#include <Arduino.h>

TaskHandle_t receiveData;


#define Running 2
#define runInterval 500
unsigned long previousMillis;
bool ledState;
bool occupancy = 1;
bool online = 1;
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
//-----------------------------Alert System------------------------------
#define vibrator 5
bool vibrationState;
unsigned long vibratorLast;

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

byte M = 0;
//----------------Load Cell-------------------------
#include "HX711.h"
#define FRONT_LEFT_DOUT_PIN 32 //Blue
#define FRONT_LEFT_SCK_PIN 33 //Green
#define FRONT_RIGHT_DOUT_PIN 19 //Orange19
#define FRONT_RIGHT_SCK_PIN 18 //Yellow18
#define BACK_LEFT_DOUT_PIN 17 // White TX
#define BACK_LEFT_SCK_PIN 16 // Brown RX
#define BACK_RIGHT_DOUT_PIN 23 //Grey
#define BACK_RIGHT_SCK_PIN 22 //Purple

HX711 BackLscale;
HX711 BackRscale;
HX711 FrontLscale;
HX711 FrontRscale;

uint8_t silent = 0;
uint8_t extend = 0;
unsigned long prevTrig = 0;
uint8_t trig = 0;
uint8_t backRightScale = 0;
uint8_t backLeftScale = 0;
uint8_t frontRightScale = 0;
uint8_t frontLeftScale = 0;
uint8_t seat = 0;
uint8_t back = 0;
uint8_t front = 0;
uint8_t left = 0;
uint8_t right = 0;
uint8_t warning = 0;
int violations = 0;
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

void receiveDataFromFirebase(){
    if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    if (Firebase.RTDB.getInt(&fbdo, "data/extended/state")) {
      if (fbdo.dataType() == "int") {
        extend = fbdo.intData();
        Serial.print("Extended time: ");
        Serial.println(extend);
      }
    }
    else {
      Serial.println(fbdo.errorReason());
    }
    if (Firebase.RTDB.getInt(&fbdo, "data/time/minutes")) {
      if (fbdo.dataType() == "int") {
        M = fbdo.intData();
        Serial.print("Minutes: ");
        Serial.println(M);
      }
    }
    else {
      Serial.println(fbdo.errorReason());
    }
    if (Firebase.RTDB.getInt(&fbdo, "data/silentMode/state")) {
      if (fbdo.dataType() == "int") {
        silent = fbdo.intData();
        Serial.print("Silent Mode: ");
        Serial.println(silent);
      }
    }
    else {
      Serial.println(fbdo.errorReason());
    }
  }
  
}

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);
  pinMode(Running,OUTPUT);
  pinMode(vibrator,OUTPUT);
  myMP3.begin(mySerial,true);
  myMP3.volume(30);
  //WifiConnect();
 //FirebaseInit();
  //configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  //WifiConnect();
  FrontLscale.begin(FRONT_LEFT_DOUT_PIN, FRONT_LEFT_SCK_PIN);
  FrontLscale.set_scale(-54824.7012);
  FrontLscale.tare();
  FrontRscale.begin(FRONT_RIGHT_DOUT_PIN, FRONT_RIGHT_SCK_PIN);
  FrontRscale.set_scale(-60354.92901);
  FrontRscale.tare();
  BackLscale.begin(BACK_LEFT_DOUT_PIN,BACK_LEFT_SCK_PIN);
  BackLscale.set_scale();
  BackLscale.set_scale(58138.37233);
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



void vibratorRun(){
  if(millis()-vibratorLast >= 1000){
    vibratorLast = millis();
    if(vibrationState==0){
      vibrationState = 1;
    }else{
      vibrationState = 0;
    }
    digitalWrite(vibrator,vibrationState);
  }
}

void sittingTimeEnforce(){
  Serial.println(M);
  
  if(M > 45){
    postureState = 5;
    M = 0;
  }
}

void getLoadCellReading(){
  Serial.println("---------------------------------------------");
  backLeftScale = BackLscale.get_units(5);
  frontLeftScale = FrontLscale.get_units(5);
  backRightScale = BackRscale.get_units(5);
  frontRightScale = FrontRscale.get_units(5);
  Serial.print("Back Left: ");
  Serial.println(backLeftScale);
  Serial.print("Back Right: ");
  Serial.println(backRightScale);
  Serial.print("Front Left: ");
  Serial.println(frontLeftScale);
  Serial.print("Front Right: ");
  Serial.println(frontRightScale);
}

void processLoadCellReading(){
  back = (backLeftScale+backRightScale)/2;
  front = (frontLeftScale+frontRightScale)/2;
  right = (backRightScale+frontRightScale)/2;
  left = (backLeftScale+frontLeftScale)/2;

  if (backLeftScale<2 && backRightScale<2 && frontLeftScale<2 && frontRightScale<2){
    Serial.println("Chair is unoccupied");
    postureState = 1;
    occupancy = 0;
  }else if((abs(backRightScale-backLeftScale) <= 4) && (abs(frontLeftScale-frontRightScale) <= 4) && (abs(left-right) <= 4) && (abs(back-front) <= 7)){
    Serial.println("Posture is correct");
    postureState = 1;
    occupancy = 1;
  }else if(abs(front-back) > 3  && (abs(left-right) <=4)){
    Serial.println("User is leaning forward");
    postureState = 3;
    occupancy = 1;
    trig++;
  }else if((backRightScale<10) && abs(back-front) <= 7){
    Serial.println("User is leaning Left");
    postureState = 4;
    occupancy = 1;
  }else if((backLeftScale<10) && abs(back-front <= 7)){  
    Serial.println("User is leaning right");
    postureState = 5;
    occupancy = 1;
  }else if(abs(front-back)<2 && (abs(left-right) <= 4)){
    Serial.println("User is leaning back");
    postureState = 2;
    occupancy = 1;
  }else if(M > 45){
    Serial.println("Sitting Time Too long");
    postureState = 6;
  }else{
    Serial.println("Posture is out of detailed range");
    postureState = 2;
  }
}

void rawLoadCellProcess(){
  Serial.println(abs((frontLeftScale+frontRightScale)-(backLeftScale+backRightScale))/2);
}

void audioAlert(){
  audioPlaying = myMP3.isPlaying();
  switch(postureState){
    case 1: //Correct Posture
      Serial.println("------------------FINAL-------------------------");
      Serial.println("Posture is correct");
    break;
    case 2: //Leaning back
      Serial.println("------------------FINAL-------------------------");
      if(silent == 0){
        if(audioPlaying == false){
          myMP3.volume(30);
          myMP3.play(1);
          trig = 0;
          Serial.println("Unsilent Back");
        }
      }
      warning++;
      if(warning>3){
        vibratorRun();
        Serial.println("Backup back");
        warning = 0;
      } 
    break;
    case 3://Leaning forward
      Serial.println("------------------FINAL-------------------------");
      if(silent == 0){
        if(audioPlaying == false){
          myMP3.volume(30);
          myMP3.play(2);
          Serial.println("Unsilent Forward");
        }
      }
      warning++;
      if(warning>3){
        vibratorRun();
        Serial.println("Backup Forward");
        warning = 0;
      } 
    break;
    case 4://Leaning left
      Serial.println("------------------FINAL-------------------------");
      if(silent == 0){
        if(audioPlaying == false){
          myMP3.volume(30);
          myMP3.play(4);
          Serial.println("Unsilent Left");
        }
      }
      warning++;
      if(warning>3){
        vibratorRun();
        Serial.println("Backup Left");
        warning = 0;
      }
    break;
    case 5://Leaning right
      Serial.println("------------------FINAL-------------------------");
      if(silent ==0){
        if(audioPlaying == false){
          myMP3.volume(30);
          myMP3.play(3);
          Serial.println("Unsilent Right");
        } 
      }
      warning++;
      if(warning>3){
        vibratorRun();
        Serial.println("Backup Right");
        warning = 0;
      }
    break;
    case 6: //Out of range
      Serial.println("------------------FINAL-------------------------");
      if(silent == 0){
        if(audioPlaying == false){
          myMP3.volume(30);
          myMP3.play(5);
        }
      }
      warning++;
      if(warning>3){
        vibratorRun();
        warning = 0;
      }
      Serial.println("Out of Range");
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

void sendDataToFirebase(){
    if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)){
      sendDataPrevMillis = millis();
      Firebase.RTDB.setInt(&fbdo, "data/violations/times", violations);  
      Firebase.RTDB.setInt(&fbdo, "data/online/state", online);
      Firebase.RTDB.setInt(&fbdo, "data/occupancy/status", occupancy);
      Firebase.RTDB.setInt(&fbdo, "data/sensor/backleft", backLeftScale);      
      Firebase.RTDB.setInt(&fbdo, "data/sensor/backright", backRightScale);
      Firebase.RTDB.setInt(&fbdo, "data/sensor/frontleft", frontLeftScale);
      Firebase.RTDB.setInt(&fbdo, "data/sensor/frontright", frontRightScale);
      Firebase.RTDB.setInt(&fbdo, "data/sensor/seat", seat);
  }
}

void loop() {
  ledBlink();
  Serial.println("Starting...");
  //getLocalTime();
  //calculateTime();
  //rawLoadCellProcess();
  sendDataToFirebase();
  receiveDataFromFirebase();
  getLoadCellReading();
  processLoadCellReading();
  audioAlert();
  //vibratorRun(); 
}