#define USE_WIFI_NINA   false
#define USE_WIFI101     true

#include <WiFiWebServer.h>
#include <ArduinoJson.h> //6.21.5
#include <Servo.h>
#include <Arduino.h>
#include <functional>

// network globals
static const char* SSID = "EEERover";
static const char* PASS = "exhibition";
static const int GROUP_NUMBER = 21;    // 0 means “no static IP”


// takes frequencies of ir and radio and returns species
const char* getRadioSpecies(float freq) {
  if (freq > 145.0 && freq < 155.0) {
    return "Zapple";
  }
  else if (freq > 95.0 && freq < 105.0) {
    return "Gribbit";
  }
  else {
    return nullptr;
  }
}

const char* getIrSpecies(float freq) {
  if (freq > 280.0 && freq < 310.0) {
    return "Snorkie";
  }
  else if (freq > 440.0 && freq < 470.0) {
    return "Wibbo";
  }
  else {
    return nullptr;
  }
}


//ULTRASONIC DETECTION CLASS - uses serial1 to communicate and decode UART signal with built in RX port
class UltrasonicDetector {
public:
  String lastDetected;  // holds the last 4‐byte word once stable

  UltrasonicDetector() {
    stableCount = 0;
    printedOnce = false;
    lastDetected = "";
    memset(currentWord, 0, sizeof(currentWord));
    memset(prevWord, 0, sizeof(prevWord));
  }

  void begin() {
    Serial1.begin(600);  // D0 as RX @ 600 baud
    Serial.println();
    Serial.println(F("Ultrasonic Detector Started"));
  }

  //set the memmory to nothing
  void reset() {
    while (Serial1.available()) {
      Serial1.read(); //flush any incoming signals
    }
    stableCount = 0;
    printedOnce = false;
    lastDetected = "";
    memset(currentWord, 0, sizeof(currentWord));
    memset(prevWord, 0, sizeof(prevWord));
  }

  //main process called in loop whenever /ultrasonic is pinged from frontend with get rq
  void process() {
    if (!Serial1.available()) return;
    int incoming = Serial1.read(); // UART standard protocol allows for us to just read incoming text
    if (incoming != 0x23) {
      return;  // skip until '#'
    }

    currentWord[0] = (char)incoming;
    for (int i = 1; i < 4; i++) {
        unsigned long timeStart = millis();
      while (!Serial1.available()) { //make a busy wait with a max timeout of 5 secs to wait for serial1 buffer
        unsigned long timeNow = millis();
        if (timeNow - timeStart >= 5000UL){
          i = 5;
          break;
        }
      } 
      currentWord[i] = (char)Serial1.read(); //read the character into the currentWord array
    }
    currentWord[4] = '\0'; //null‐terminate

    // stableCount tracks how many times the SAME WORD has been recieved by the ultrasonic detector
    // it must reach at least 5 times to be passed as valid
    if (stableCount == 0) {
      memcpy(prevWord, currentWord, sizeof(currentWord));
      stableCount = 1;
      printedOnce = false;
    } 
    else {
      if (strncmp(currentWord, prevWord, 4) == 0) { //strncmp returns 0 if the first n=4 characters are equal of the 2 null terminated strings
        stableCount++;
      } 
      else {
        memcpy(prevWord, currentWord, sizeof(currentWord));// if they dont match move on with new one as last word to compare to
        stableCount = 1;
        printedOnce = false;
      }
    }

    if (stableCount >= 5 && !printedOnce) {
      lastDetected = String(currentWord); //this is read later on and returned to the user
      printedOnce = true;
    }
  }

private:
  char currentWord[5];
  char prevWord[5];
  uint8_t stableCount;
  bool printedOnce;
};


//RADIO DETECTOR CLASS - 
class RadioDetector {
public:
  static constexpr uint8_t RADIO_PIN = 2; //constexpr means the compiler will write it as 2 everywhere in the code - no memory storage goes on
  String lastDetected;  // holds last identified species

  RadioDetector() {
    freqSum = 0.0f;
    sampleCnt = 0;
    freqSmoothed = 0.0f;
    lastDetected = "";
  }

  void begin() {
    pinMode(RADIO_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(RADIO_PIN), RadioDetector::onRisingEdge, RISING); //inbuilt arduino func attachInterrupt to pin 2 for every risigng edge
    //this calls orRisingEdge func below when  interupt commences
    Serial.println();
    Serial.println(F("Radio Detector Initialized on PIN 2"));
  }

 // clear any pending interrupt driven data by switching on and off the interupt service and resetting all variables to original vals
  void reset() {
    noInterrupts();
    newPeriodAvail = false;
    periodUs = 0;
    lastEdgeTime = 0;
    interrupts();
    freqSum = 0.0f;
    sampleCnt = 0;
    freqSmoothed = 0.0f;
    lastDetected = "";
  }

  //called in loop when toggled from frontend /radiowave gets samples >=8 and averages them to give frequency of wave.
  //this is then passed into helper function at the top which decodes the species
  void process() {
    if (!newPeriodAvail) return;

    noInterrupts();
    unsigned long capturedPeriod = periodUs;
    newPeriodAvail = false;
    interrupts();

    float instFreq = 1e6 / float(capturedPeriod);
    freqSum += instFreq;
    sampleCnt++;

    if (sampleCnt >= 8) {
      freqSmoothed = freqSum / sampleCnt;
      freqSum = 0.0f;
      sampleCnt = 0;
      const char* species = getRadioSpecies(freqSmoothed);
      if (species != nullptr) {
        lastDetected = String(species);
      }
    }
  }

private:
  static volatile unsigned long lastEdgeTime;
  static volatile unsigned long periodUs;
  static volatile bool newPeriodAvail;

  float freqSum;
  int sampleCnt;
  float freqSmoothed;

  static void onRisingEdge() {
    unsigned long now = micros();
    if (lastEdgeTime != 0) {
      periodUs = now - lastEdgeTime;
      newPeriodAvail = true;
    }
    lastEdgeTime = now;
  }
};

//static variables need storage location
volatile unsigned long RadioDetector::lastEdgeTime = 0;
volatile unsigned long RadioDetector::periodUs = 0;
volatile bool RadioDetector::newPeriodAvail = false;


//IR DETECTOR CLASS - works in exactly the same way as the class above (Radio Detector) but for IR signals
class IRDetector {
public:
  static constexpr uint8_t IR_PIN = 3;
  String lastDetected;  // holds last identified species

  IRDetector() {
    freqSum = 0.0f;
    sampleCnt = 0;
    freqSmoothed = 0.0f;
    lastDetected = "";
  }

  void begin() {
    pinMode(IR_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(IR_PIN), IRDetector::onRisingEdge, RISING);
    Serial.println(F("IR Detector Initialized on PIN 3"));
  }

  void reset() {
    noInterrupts();
    newPeriodAvail = false;
    periodUs = 0;
    lastEdgeTime = 0;
    interrupts();
    freqSum = 0.0f;
    sampleCnt = 0;
    freqSmoothed = 0.0f;
    lastDetected = "";
  }

  void process() {
    if (!newPeriodAvail) return;

    noInterrupts();
    unsigned long capturedPeriod = periodUs;
    newPeriodAvail = false;
    interrupts();

    float instFreq = 1e6 / float(capturedPeriod);
    freqSum += instFreq;
    sampleCnt++;

    if (sampleCnt >= 8) {
      freqSmoothed = freqSum / sampleCnt;
      freqSum = 0.0f;
      sampleCnt = 0;
      const char* species = getIrSpecies(freqSmoothed);
      if (species != nullptr) {
        lastDetected = String(species);
      }
    }
  }

private:
  static volatile unsigned long lastEdgeTime;
  static volatile unsigned long periodUs;
  static volatile bool newPeriodAvail;

  float freqSum;
  int sampleCnt;
  float freqSmoothed;

  static void onRisingEdge() {
    unsigned long now = micros();
    if (lastEdgeTime != 0) {
      periodUs = now - lastEdgeTime;
      newPeriodAvail = true;
    }
    lastEdgeTime = now;
  }
};

volatile unsigned long IRDetector::lastEdgeTime = 0;
volatile unsigned long IRDetector::periodUs = 0;
volatile bool IRDetector::newPeriodAvail = false;

// MAGNETISM DETECTOR CLASS - samples the analog pin 5 times same as ultrasonic
class MagnetismDetector {
public:
  static constexpr uint8_t MAG_PIN = A0; //apparently u can do this lol
  String lastDetected; // holds last state
  MagnetismDetector() {
    stableCount = 0;
    prevState[0] = '\0';
    lastDetected = "";
  }

  //initialise pin mode A0
  void begin() {
    pinMode(MAG_PIN, INPUT);
  }

  //resets all important vars
  void reset() {
    stableCount = 0;
    prevState[0] = '\0';
    lastDetected = "";
  }

  //looped func
  void process() {
    int mag_voltage = analogRead(MAG_PIN);
    Serial.println(mag_voltage);
    //determine current instantaneous state based on thresholds:
    const char* currentState;
    if (mag_voltage >= 1000) {
      currentState = "Down";
    }
    else if (mag_voltage <= 550) {
      currentState = "Up";
    }
    else {
      currentState = "Idle";
    }

    // same logic as ultrasonic signal for previous state and correct sampling 5 times
    if (stableCount == 0) {
      strcpy(prevState, currentState);
      stableCount = 1;
    }
    else {
      if (strcmp(currentState, prevState) == 0) {
        stableCount++;
      }
      else {
        strcpy(prevState, currentState);
        stableCount = 1;
      }
    }

    if (stableCount >= 5 && lastDetected != String(prevState)) {
      lastDetected = String(prevState);
    }
  }

private:
  int stableCount;     
  char prevState[6];      
};

//STATISTICS OBSERVER CLASS - gets feedback from all detector classes and stores them in variables in the class
//using struct as its conventional to use structs for data i think
struct StatsObserver {
public:
  char stats[4][16];
  
  void updateStats() {
    strncpy(stats[0], _ultrasonicMessage, sizeof(stats[0]) - 1);
    stats[0][sizeof(stats[0]) - 1] = '\0';
    strncpy(stats[1], _irMessage, sizeof(stats[1]) - 1);
    stats[1][sizeof(stats[1]) - 1] = '\0';
    strncpy(stats[2], _radiowaveMessage, sizeof(stats[2]) - 1);
    stats[2][sizeof(stats[2]) - 1] = '\0';
    strncpy(stats[3], _magneticMessage, sizeof(stats[3]) - 1);
    stats[3][sizeof(stats[3]) - 1] = '\0';

  }

  void updateUltrasonicStat (const char* msg){
    strncpy(_ultrasonicMessage, msg, sizeof(_ultrasonicMessage) - 1);
    _ultrasonicMessage[sizeof(_ultrasonicMessage) - 1] = '\0';
  }

  void updateIRStat (const char* msg){
    strncpy(_irMessage, msg, sizeof(_irMessage) - 1);
    _irMessage[sizeof(_irMessage) - 1] = '\0';
  }
  
  void updateRadiowaveStat (const char* msg){
    strncpy(_radiowaveMessage, msg, sizeof(_radiowaveMessage) - 1);
    _radiowaveMessage[sizeof(_radiowaveMessage) - 1] = '\0';
  }
  
  void updateMagnetismStat (const char* msg){
    strncpy(_magneticMessage, msg, sizeof(_magneticMessage) - 1);
    _magneticMessage[sizeof(_magneticMessage) - 1] = '\0';
  }
  

private:
  char _ultrasonicMessage[16];
  char _irMessage[16];
  char _radiowaveMessage[16];
  char _magneticMessage[16];

};


//MAIN SERVER CLASS - bundles together the wifi network, requests and instances of classes above
class EEERoverServer {
public:
  // toggles for all sensors
  bool ultrasonicToggle;
  bool magnetismToggle; // (unused for now)
  bool irToggle;
  bool radiowaveToggle;

  EEERoverServer(const char* ssid, const char* pass, int groupNum):
    _ssid(ssid),
    _pass(pass),
    _groupNumber(groupNum),
    _server(80),
    ultrasonicToggle(false),
    magnetismToggle(false),
    irToggle(false),
    radiowaveToggle(false),
    _lastWifiCheck(0)  
  {
    for (int i = 0; i < 4; i++) {
      motorValues[i] = 1500;
    }
  }

  void begin() {

    //setup servo motors
    const int topLeftWheelPin = 13;
    const int topRightWheelPin = 12;
    const int bottomLeftWheelPin = 11;
    const int bottomRightWheelPin = 10;

    topLeftWheelServo.attach(topLeftWheelPin);
    topRightWheelServo.attach(topRightWheelPin);
    bottomLeftWheelServo.attach(bottomLeftWheelPin);
    bottomRightWheelServo.attach(bottomRightWheelPin);

    //make connection with usb to serial monitor for debugging
    Serial.begin(9600);
    while (!Serial && millis() < 10000) { } //wait 10 secs max for start
    Serial.println();
    Serial.println(F("Starting EEERover JSON WebServer"));

   //check for actual hardware
    while (WiFi.status() == WL_NO_SHIELD) { 
      delay(3000);
      Serial.println(F("ERROR: WiFi shield not present")); 
    }

    // ip config if in lab
    if (_groupNumber > 0 && _groupNumber < 255) {
      IPAddress ip(192, 168, 0, _groupNumber + 1);
      IPAddress gw(192, 168, 0, 1);
      IPAddress sn(255, 255, 255, 0);
      WiFi.config(ip, gw, sn);
      Serial.print(F("Static IP configured: "));
      Serial.println(ip);
    }

    //start wifi connection
    Serial.print(F("Connecting to SSID: "));
    Serial.println(_ssid);
    while (WiFi.begin(_ssid, _pass) != WL_CONNECTED) {
      delay(500);
      Serial.print('.');
    }
    Serial.println();
    Serial.print(F("Connected to IP address: "));
    Serial.println((IPAddress)WiFi.localIP());

    // initialise all detectors
    ultrasonicDetector.begin();
    radioDetector.begin();
    irDetector.begin();
    magnetismDetector.begin();

    //create HTTP endpoints
    //each line is basically what to do when x happens e.g:
    //on /motors post request call handleMotors function from this instance of this class
    _server.on("/motors", HTTP_POST,std::bind(&EEERoverServer::handleMotors, this));
    _server.on("/ultrasonic", HTTP_GET,std::bind(&EEERoverServer::handleUltrasonic, this));
    _server.on("/magnetic", HTTP_GET,std::bind(&EEERoverServer::handleMagnetic, this));
    _server.on("/IR", HTTP_GET,std::bind(&EEERoverServer::handleIR, this));
    _server.on("/radiowaves", HTTP_GET,std::bind(&EEERoverServer::handleRadiowaves, this));
    _server.on("/stats", HTTP_GET,std::bind(&EEERoverServer::handleStats, this));
    _server.on("/*", HTTP_OPTIONS,std::bind(&EEERoverServer::handleOptions, this));
    _server.on("/ping", HTTP_GET, [this]() {
      sendCORS();
      _server.send(200, "text/plain", "pong");
    });
    _server.onNotFound(std::bind(&EEERoverServer::handleNotFound, this));

    _server.begin();
    Serial.println(F("HTTP server started"));
  }

  void run() {
    // 0.) Ensure wifi is still connected to the internet
    maintainWiFi();

    // 1.) Handle incoming HTTP requests
    _server.handleClient();

    // 2.) Call detectors if their toggles are enabled, then collect results
    if (ultrasonicToggle) {
      ultrasonicDetector.process();
      if (ultrasonicDetector.lastDetected.length() > 0) {
        statsObserver.updateUltrasonicStat(ultrasonicDetector.lastDetected.c_str()); //!!
        ultrasonicDetector.lastDetected = "";
      }
    }
    if (radiowaveToggle) {
      radioDetector.process();
      if (radioDetector.lastDetected.length() > 0) {
        statsObserver.updateRadiowaveStat(radioDetector.lastDetected.c_str());
        radioDetector.lastDetected = "";
      }
    }
    if (irToggle) {
      irDetector.process();
      if (irDetector.lastDetected.length() > 0) {
        statsObserver.updateIRStat(irDetector.lastDetected.c_str());
        irDetector.lastDetected = "";
      }
    }if (magnetismToggle) {
      magnetismDetector.process();
      if (magnetismDetector.lastDetected.length() > 0) {
        statsObserver.updateMagnetismStat(magnetismDetector.lastDetected.c_str());
        magnetismDetector.lastDetected = "";
      }
    }

    // 3) Drive servos according to motorValues array
    topLeftWheelServo.writeMicroseconds(motorValues[0]);
    topRightWheelServo.writeMicroseconds(motorValues[1]);
    bottomLeftWheelServo.writeMicroseconds(motorValues[2]);
    bottomRightWheelServo.writeMicroseconds(motorValues[3]);

    statsObserver.updateStats();
  }


//everything below here continue to analyse
private:
  unsigned long _lastWifiCheck;
  const char* _ssid;
  const char* _pass;
  int _groupNumber;
  WiFiWebServer _server;
  int motorValues[4];

  Servo topLeftWheelServo;
  Servo topRightWheelServo;
  Servo bottomLeftWheelServo;
  Servo bottomRightWheelServo;

  UltrasonicDetector ultrasonicDetector;
  RadioDetector radioDetector;
  IRDetector irDetector;
  MagnetismDetector magnetismDetector;

  StatsObserver statsObserver;

  void maintainWiFi() {
    unsigned long now = millis();
    // check every 10 seconds:
    if (now - _lastWifiCheck < 10000UL) return;
    _lastWifiCheck = now;

    if (WiFi.status() != WL_CONNECTED) {
      Serial.print(F("Wi-Fi lost, trying to reconnect… "));
      WiFi.begin(_ssid, _pass);
      // block until we do get back online (you can shorten this loop or give up after N tries)
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print('.');
      }
      Serial.println();
      Serial.print(F("Reconnected to IP address: "));
      Serial.println(WiFi.localIP());
    }
  }

  void sendCORS() {
    _server.sendHeader("Access-Control-Allow-Origin",  "*");
    _server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    _server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  }

  // POST handler for motor controls
  void handleMotors() {
    Serial.println("Received POST /motors");
    const int FastForwardRPM = 1700;
    const int SlowForwardRPM = 1550;
    const int BackwardRPM = 1200;
    const int StopRPM = 1500;

    sendCORS();

    StaticJsonDocument<200> doc;
    String json = _server.arg("plain");
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
      _server.send(400, "application/json", "{\"error\":\"invalid JSON\"}");
      return;
    }

    JsonArray cmds = doc["commands"].as<JsonArray>();
    if (!cmds) {
      _server.send(400, "application/json", "{\"error\":\"missing commands array\"}");
      return;
    }

    int motorIndex = 0;
    for (JsonVariant v : cmds) {
      const char* cmd = v.as<const char*>();
      if (strcmp(cmd,"FF") == 0) motorValues[motorIndex] = FastForwardRPM;
      else if (strcmp(cmd,"SF") == 0) motorValues[motorIndex] = SlowForwardRPM;
      else if (strcmp(cmd,"B") == 0) motorValues[motorIndex] = BackwardRPM;
      else if (strcmp(cmd,"S") == 0) motorValues[motorIndex] = StopRPM;
      else {
        Serial.println(F("Error: unexpected motor command"));
      }
      Serial.print(cmd);
      Serial.print(F(" -> "));
      Serial.println(motorValues[motorIndex]);
      Serial.println();
      motorIndex++;
      if (motorIndex >= 4) break;
    }

    _server.send(200, "application/json", "{\"status\":\"ok\"}");
  }

  // get handler for ultrasonic
  void handleUltrasonic() {
    Serial.println("Received Get /ultrasonic");
    ultrasonicToggle = !ultrasonicToggle;
    if (ultrasonicToggle) {
      ultrasonicDetector.reset();
    }
    sendCORS();
    StaticJsonDocument<64> resp;
    resp["ultrasonic"] = ultrasonicToggle;
    String out;
    serializeJson(resp, out);
    _server.send(200, "application/json", out);
  }

  // get handler for magnetic 
  void handleMagnetic() {
    Serial.println("Received get /magnetic");
    magnetismToggle = !magnetismToggle;
    if (magnetismToggle) {
      magnetismDetector.reset();
    }
    sendCORS();
    StaticJsonDocument<64> resp;
    resp["magnetism"] = magnetismToggle;
    String out;
    serializeJson(resp, out);
    _server.send(200, "application/json", out);
  }

  // get handler for infrared
  void handleIR() {
    Serial.println("Received get /ir");
    irToggle = !irToggle;
    if (irToggle) {
      irDetector.reset();
    }
    sendCORS();
    StaticJsonDocument<64> resp;
    resp["IR"] = irToggle;
    String out;
    serializeJson(resp, out);
    _server.send(200, "application/json", out);
  }

  // get handler for radio waves
  void handleRadiowaves() {
    Serial.println("Received get /radiowaves");
    radiowaveToggle = !radiowaveToggle;
    if (radiowaveToggle) {
      radioDetector.reset();
    }
    sendCORS();
    StaticJsonDocument<64> resp;
    resp["radiowaves"] = radiowaveToggle;
    String out;
    serializeJson(resp, out);
    _server.send(200, "application/json", out);
  }

  // get handler for statistics object that posts array of stats collected with json to /stats
  // returns JSON: { "stats": [ "entry1", "entry2", ...] }
  void handleStats() {
    sendCORS();
    StaticJsonDocument<512> doc;  // might need to add some storage but should be chill
    JsonArray arr = doc.createNestedArray("stats");
    for (int i = 0; i < 4; i++) {
      arr.add(statsObserver.stats[i]);
    }
    String out;
    serializeJson(doc, out);
    _server.send(200, "application/json", out);
  }

  // options handler - cors headers
  void handleOptions() {
    Serial.println("sent options");
    sendCORS();
    _server.send(204);
  }

  // error msg handler
  void handleNotFound() {
    sendCORS();
    String message = "File Not Found\n\n";
    message += "URI: "   + _server.uri() + "\n";
    message += "Method: ";
    message += (_server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: " + String(_server.args()) + "\n";
    for (uint8_t i = 0; i < _server.args(); i++) {
      message += " " + _server.argName(i) + ": " + _server.arg(i) + "\n";
    }
    _server.send(404, "text/plain", message);
  }
};

//— Create one global instance (using your SSID, PASS, GROUP_NUMBER) ——
EEERoverServer rover(SSID, PASS, GROUP_NUMBER);

void setup() {
  rover.begin();
}

void loop() {
  rover.run();
}
