
//-----------------------------------------------------------------------------------------
//LIBRARY IMPORTS -- 1364346
//-----------------------------------------------------------------------------------------
#include <BLEDevice.h> //importing bluetooth library
//#include <WiFi.h> //importing wifi library 
#include <WebServer.h> //importing webserver library to setup a server on the ESP
#include <OneWire.h>
#include <DallasTemperature.h>


//-----------------------------------------------------------------------------------------
//VARIABLES
//-----------------------------------------------------------------------------------------

//Defined variables
//-----------------------------------------------------------------------------------------
//Bluetooth
#define TARGET_rssi -100 //rssi limite para ligar o relê.
#define MAX_MISSING_TIME 300000 //300.000ms = 300s = 5min

//WiFi
WiFiServer server(80);

//Temperature Sensor
#define ONE_WIRE_BUS 0// Data wire is plugged TO GPIO 0
OneWire oneWire(ONE_WIRE_BUS);// Setup a oneWire instance to communicate with OneWire devices
DallasTemperature sensors(&oneWire);// Pass our oneWire reference to Dallas Temperature. 
DeviceAddress tempDeviceAddress; // We'll use this variable to store a found device address

//Humidity Sensor
#define humiditySensor 35// Humidity Sensor is plugged TO GPIO 4

//Light Sensor
#define lightSensorOut 34// Outdoor light sensor is plugged TO GPIO 34
#define lightSensorIn 32// Indoor light sensor is plugged TO GPIO 23

//Output
#define windowClose 14// Rele to close window is plugged TO GPIO 14
#define windowOpen 12// Rele to open window is plugged TO GPIO 12
#define infraRed 2// Infrared to control Air is plugged TO GPIO 2

//Bluetooth Variables
//-----------------------------------------------------------------------------------------
BLEScan* pBLEScan; //Variável que irá guardar o scan
uint64_t lastFoundTime[2] = {0, 0}; //Tempo em que encontrou o iTag pela última vez
uint64_t DeltaLFT[2] = {0, 0}; 
uint64_t now = 0;
int BLErssi = 0;
int BLEDevice = 0;

int i = 0; //generic variable used as a pointer on For's functions, DO NOT USE TO SAVE INFO

//Bluetooth Users Setup - Logical part
int numberOfUsers = 2;
char ADDRESS[2][18] = {"d9:eb:46:eb:42:f0", "d5:ff:84:84:ec:d5"};
char User[2][20] = {"Lucas", "Renan"};
int UserRSSI[2] = {0, 0};
boolean hasMR = false;
int MRUser = -1;
int LRUser = -1;

//Wifi Variables
//-----------------------------------------------------------------------------------------
const char* ssid = "REDE.CASA"; //SSID address of the WiFi connection
const char* password = "Na0Interess4"; //password of the WiFi
boolean wifiConnected = false; //flag for the wifi connection status

//Temperature Sensor Variables
//-----------------------------------------------------------------------------------------
int numberOfDevices;// Number of temperature devices found
float temperatureIn = 0;
float temperatureOut = 0;

//Humidity Sensor Variables
//-----------------------------------------------------------------------------------------
int humidityValue = 0;
int wetValue = 3000;

//Light Sensor Variables
//-----------------------------------------------------------------------------------------
int lightValueIn = 0;
int lightValueOut = 0;
int nightValue = 200;

//Logical Variables
//-----------------------------------------------------------------------------------------
boolean windowAuto[2] = {false,false};
boolean airAuto[2] = {false,false};
boolean raining = false;
boolean opened = false;
int windowDelay = 1500;
int airDelay = 100;

//User preferences
//-----------------------------------------------------------------------------------------
int userTemperature[2] = {23, 25};
int deltaTempConfort[2] = {2, 2};
int userLight[2] = {2000, 2500};
int deltaLightConfort[2] = {400, 400};

//-----------------------------------------------------------------------------------------
//FUNCTIONS
//-----------------------------------------------------------------------------------------

//Setup Functions
//-----------------------------------------------------------------------------------------
char connectWifi(){
  i = 15;
  while (WiFi.status() != WL_CONNECTED && i>0) {
    delay(500);
    Serial.print(".");
    if (WiFi.status() == WL_CONNECTED){
      wifiConnected = true;
      Serial.println("connected");
    }
    Serial.println(i);
    i--;
  }
    return WiFi.status();
}


//Read Connection Functions
//-----------------------------------------------------------------------------------------
//Callback das chamadas ao scan - Bluetooth
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    //Se for o dispositivo que esperamos
    for(i = 0; i < numberOfUsers; i++){ 
      if(advertisedDevice.getAddress().toString() == ADDRESS[i]){//Marcamos como encontrado, paramos o scan e guardamos o rssi
        BLEDevice = i;
        BLErssi = advertisedDevice.getRSSI();
        UserRSSI[BLEDevice] = BLErssi;
        if(BLErssi > TARGET_rssi){
          lastFoundTime[BLEDevice] = millis(); //Tempo em milissegundos de quando encontrou
        }
      }
    }
  }
};

//Function to resquet temperature 
float requestTemp(int sensor){
  sensors.requestTemperatures();
  if(sensors.getAddress(tempDeviceAddress, sensor)){
    float tempC = sensors.getTempC(tempDeviceAddress);
    return tempC;
  }
}


//Core Functions
//-----------------------------------------------------------------------------------------
//Check Wifi Connection & Conditions
//-----------------------------------------------------------------------------------------
void checkWifi(){
  
  if (!wifiConnected){
  Serial.println("");
  Serial.println("Re-trying to connect..");
  connectWifi();
  }
  
  else {
  WiFiClient client = server.available();
  
  if (client) {
    Serial.println("New Client.");
    String currentLine = "";
    
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.print("<b>Window options:</b><br>");
            client.print("Click <a href=\"/WindowOpen\">here</a> to open the Window.<br>");
            client.print("Click <a href=\"/WindowClose\">here</a> to close the Window.<br><br>");
//            client.print("*");
            client.print("<b>Air Conditioner options:</b><br>");
            client.print("Click <a href=\"/AirOn\">here</a> to turn the Air Conditioner on.<br>");
            client.print("Click <a href=\"/AirOff\">here</a> to turn the Air Conditioner off.<br><br>");
//            client.print("*");
            client.print("<b>Temperature options:</b><br>");
            client.print("Click <a href=\"/TempUp\">here</a> to increase the temperature.<br>");
            client.print("Click <a href=\"/TempDown\">here</a> to decrease the temperature.<br><br>");
//            client.print("*");
            client.print("<b>Automation options:</b><br>");
            client.print("Click <a href=\"/WindowAutoOn\">here</a> to turn the Window Automation on.<br>");
            client.print("Click <a href=\"/WindowAutoOff\">here</a> to turn the Window Automation off.<br>");
            client.print("Click <a href=\"/AirAutoOn\">here</a> to turn the Air Conditioner Automation on.<br>");
            client.print("Click <a href=\"/AirAutoOff\">here</a> to turn the Air Conditioner Automation off.<br><br>");
//            client.print("*");
            client.print("<br><b>Last internal temperature: </b>");
            client.print(temperatureIn);
            client.print(" C<br><b>Last outdoor temperature: </b>");
            client.print(temperatureOut);
            client.print(" C<br><br><b>Most relevant user:</b> ");
            client.print(User[MRUser]);
            client.print("<br><b>User preferred temperature:</b> ");
            client.print(userTemperature[MRUser]);
            client.print(" C");
            client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
        if (currentLine.endsWith("GET /WindowOpen")) {
          Serial.println("Opening window...");
          windowAuto[MRUser] = false;
          airAuto[MRUser] = false;
          manualChangeWindowStatus(true);
          updatePreferences();
        }
        if (currentLine.endsWith("GET /WindowClose")) {
          Serial.println("Closing window...");
          windowAuto[MRUser] = false;
          manualChangeWindowStatus(false);
          updatePreferences();
        }
        if (currentLine.endsWith("GET /AirOn")) {
          Serial.println("Turning air on");
          airAuto[MRUser] = false;
          windowAuto[MRUser] = false;
          manualChangeAirStatus(true);
          updatePreferences();
        }
        if (currentLine.endsWith("GET /AirOff")) {
          Serial.println("Turning air off");
          airAuto[MRUser] = false;
          manualChangeAirStatus(false);
          updatePreferences();
        }
        if (currentLine.endsWith("GET /TempUp")) {
          Serial.println("Turning air off");
          userTemperature[MRUser] ++;
        }
        if (currentLine.endsWith("GET /TempDown")) {
          Serial.println("Turning air off");
          userTemperature[MRUser] --;
        }
        if (currentLine.endsWith("GET /WindowAutoOn")) {
          Serial.println("Auto window control on");
          windowAuto[MRUser] = true;
        }
        if (currentLine.endsWith("GET /WindowAutoOff")) {
          Serial.println("Auto window control off");
          windowAuto[MRUser] = false;
        }
        if (currentLine.endsWith("GET /AirAutoOn")) {
          Serial.println("Auto air control and window on");
          windowAuto[MRUser] = true;
          airAuto[MRUser] = true;
        }
        if (currentLine.endsWith("GET /AirAutoOff")) {
          Serial.println("Auto air control off");
          airAuto[MRUser] = false;
        }
      }
    }
    client.stop();
    Serial.println("Client Disconnected.");
  }
  }
}


//Check bluetooth status - Core Functions
//-----------------------------------------------------------------------------------------
void checkBLE(){
  

  pBLEScan->start(1, false);
  now = millis(); //Tempo em milissegundos desde o boot
  DeltaLFT[0] = now - lastFoundTime[0];
  DeltaLFT[1] = now - lastFoundTime[1];
  
  if((!hasMR) && (DeltaLFT[0] != now) && ((lastFoundTime[0] < lastFoundTime[1]) || lastFoundTime[1] == 0)){
    MRUser = 0;
    LRUser = 1;
    hasMR = true; 
  } 
    else if((!hasMR) && (DeltaLFT[1] != now) && ((lastFoundTime[1] < lastFoundTime[0]) || lastFoundTime[0] == 0)){
    MRUser = 1;
    LRUser = 0;
    hasMR = true; 
  } 
    else if((DeltaLFT[MRUser] < now) && (DeltaLFT[MRUser] > MAX_MISSING_TIME)){
    lastFoundTime[MRUser] = 0;
    
      if((DeltaLFT[LRUser] < now) && (DeltaLFT[LRUser] < MAX_MISSING_TIME)){
        MRUser = LRUser;
        LRUser = -1;  
      }
      else {
        lastFoundTime[LRUser] = 0;
        MRUser = -1;
        LRUser = -1;
        hasMR = false;
      }
    }
}

//Open/Close window - Core Functions
//-----------------------------------------------------------------------------------------
void changeWindowStatus(boolean statusOpen){
  if (statusOpen != opened){
   digitalWrite(windowOpen, LOW);
   digitalWrite(windowClose, LOW);
   delay(500);
  if (statusOpen){ 
    digitalWrite(windowOpen, HIGH);
    opened = true;
    } else {
      digitalWrite(windowClose, HIGH);
//      Serial.println("fechando...");
      opened = false;
      }
  delay(windowDelay);
  }
}

void autoChangeWindowStatus(boolean statusOpen){
  if(windowAuto[MRUser]){
    if(raining){
      changeWindowStatus(false);
    } else{
    changeWindowStatus(statusOpen);
    }
  }
}

void manualChangeWindowStatus(boolean statusOpen){
    changeWindowStatus(statusOpen);
}

//On/Off Air Conditioner - Core Functions
//-----------------------------------------------------------------------------------------
void changeAirStatus(boolean statusOn){
  if (statusOn){ 
    digitalWrite(infraRed, HIGH);
    } else {
      digitalWrite(infraRed, LOW);
      }
  delay(airDelay);
}

void autoChangeAirStatus(boolean statusOn){
  if(airAuto[MRUser]){
    changeAirStatus(statusOn);
  }
}

void manualChangeAirStatus(boolean statusOn){
    changeAirStatus(statusOn);
}

//User preferences - Core Functions
//-----------------------------------------------------------------------------------------
void updatePreferences(){//set timer to update after 3 minutes
  int temp = userTemperature[MRUser];
  int light = userLight[MRUser];
  userTemperature[MRUser] = (temperatureIn + temp)/2;
  userLight[MRUser] = (lightValueIn + light)/2;
}

//Raining - Core Functions
//-----------------------------------------------------------------------------------------
void checkRain(){
  humidityValue = analogRead(humiditySensor);
  if (humidityValue < wetValue){
    Serial.print("Raining: ");
    Serial.println(humidityValue);
    raining = true;
    autoChangeWindowStatus(false);
  } else {
    raining = false;
  }
}

//Light and Temperature - Core Functions
//-----------------------------------------------------------------------------------------
void readLight(){
  lightValueIn = analogRead(lightSensorIn);
  lightValueOut = analogRead(lightSensorOut);
}

void readTemp(){
  temperatureIn = requestTemp(0);//read the indoor temperature
  temperatureOut = requestTemp(1);//read the outdoor temperature
}

//Conformt - Core Functions
//-----------------------------------------------------------------------------------------
void checkConfort(){
  readTemp();
  readLight();
  if(MRUser != -1){
    int deltaTempIn = abs(temperatureIn - userTemperature[MRUser]);
    int deltaTempOut = abs(temperatureOut - userTemperature[MRUser]);
    int deltaLightIn = abs(lightValueIn - userLight[MRUser]);
    int deltaLightOut = abs(lightValueOut - userLight[MRUser]);
      Serial.println(deltaTempIn);
      Serial.println(deltaTempOut);
      Serial.println(deltaLightIn);
      Serial.println(deltaLightOut);
    

    if(lightValueOut < nightValue){//is night
      autoChangeWindowStatus(false);
      Serial.println("Night...");
      if((airAuto[MRUser] != -1) && (deltaTempIn > deltaTempConfort[MRUser])){
        autoChangeAirStatus(true);
        Serial.println("Air ON");
      }
    } else {//day
        if((airAuto[MRUser] != -1) && (deltaTempIn > deltaTempConfort[MRUser])){
          if((temperatureIn > temperatureOut) && (deltaTempOut > deltaTempConfort[MRUser])){
            autoChangeWindowStatus(true);
            Serial.println("Fresh Air");
          } else {
            if((temperatureIn > userTemperature[MRUser])){
            Serial.println("Cooled Air");
            autoChangeWindowStatus(false);
            autoChangeAirStatus(true);
            } else {//((temperatureIn < userTemperature[MRUser]) && (temperatureOut > userTemperature[MRUser]))
            Serial.println("Heated Air");
            autoChangeWindowStatus(false);
            autoChangeAirStatus(true);
            }
            }
        } else if(deltaLightIn > deltaLightConfort[MRUser]){
            if((lightValueIn < lightValueOut) && (deltaLightOut > deltaLightConfort[MRUser])){
              autoChangeWindowStatus(true);
              Serial.println("Fresh Light");
            } else {
              autoChangeWindowStatus(true);
              Serial.println("Default - Window Opened");
            }
          }
      }
  }  
}

//-----------------------------------------------------------------------------------------
//BASIC SETUP
//-----------------------------------------------------------------------------------------

void setup()
{

  //should be deleted when completed, serial will be just used to see if the program is working as planned
  Serial.begin(115200);//TO BE DELETED
  Serial.println();//TO BE DELETED
  Serial.print("Conectando-se a ");//TO BE DELETED
  Serial.println(ssid);//TO BE DELETED

//WiFi Setup 
  WiFi.begin(ssid, password);
  connectWifi();
if (wifiConnected){
  Serial.println("");//TO BE DELETED
  Serial.println("WiFi conectado.");//TO BE DELETED
  Serial.println("Endereço de IP: ");//TO BE DELETED
  Serial.println(WiFi.localIP());//TO BE DELETED
  server.begin();
  } 
  else {//TO BE DELETED
  Serial.println("");//TO BE DELETED
  Serial.println("WiFi não conectado.");//TO BE DELETED
  }//TO BE DELETED


//Bluetooth Setup 
  BLEDevice::init(""); 
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);

//Temperature Sensor  
  sensors.begin();// Start up the library
  numberOfDevices = sensors.getDeviceCount();// Grab a count of devices on the wire
  temperatureIn = requestTemp(0);//setup the base temperature
  temperatureOut = requestTemp(1);//setup the base temperature
  
//Physical output setup       
  pinMode(windowClose, OUTPUT);// should be the rele output
  pinMode(windowOpen, OUTPUT);// should be the rele output
  digitalWrite(windowClose, LOW);
  digitalWrite(windowOpen, LOW);
}


//-----------------------------------------------------------------------------------------
//LOOP
//-----------------------------------------------------------------------------------------
void loop()
{   
  checkRain();
  checkWifi();
  checkBLE();
  checkConfort();
  Serial.println("Loop...");
}
