#include <esp-fs-webserver.h>  // https://github.com/cotestatnt/esp-fs-webserver

#include <Wire.h>
#include <FS.h>
#include <LittleFS.h>
#include <Adafruit_PWMServoDriver.h>
#define FILESYSTEM LittleFS

FSWebServer myWebServer(FILESYSTEM, 80);

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

#define SERVOMIN  125 // This is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  550 // This is the 'maximum' pulse length count (out of 4096)
#define USMIN  600 // This is the rounded 'minimum' microsecond length based on the minimum pulse of 150
#define USMAX  2400 // This is the rounded 'maximum' microsecond length based on the maximum pulse of 600
#define SERVO_FREQ 50 // Analog servos run at ~50 Hz updates

// In order to set SSID and password open the /setup webserver page
// const char* ssid;
// const char* password;

uint8_t ledPin = LED_BUILTIN;
bool apMode = false;

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

////////////////////////////////  Filesystem  /////////////////////////////////////////
void startFilesystem() {
  // FILESYSTEM INIT
  if ( !FILESYSTEM.begin()) {
    Serial.println("ERROR on mounting filesystem. It will be formmatted!");
    FILESYSTEM.format();
    ESP.restart();
  }
  myWebServer.printFileList(LittleFS, Serial, "/", 2);
}

/*
* Getting FS info (total and free bytes) is strictly related to
* filesystem library used (LittleFS, FFat, SPIFFS etc etc) and ESP framework
* ESP8266 FS implementation has methods for total and used bytes (only label is missing)
*/
#ifdef ESP32
void getFsInfo(fsInfo_t* fsInfo) {
	fsInfo->fsName = "LittleFS";
	fsInfo->totalBytes = LittleFS.totalBytes();
	fsInfo->usedBytes = LittleFS.usedBytes();
}
#else
void getFsInfo(fsInfo_t* fsInfo) {
	fsInfo->fsName = "LittleFS";
}
#endif

////////////////////////////  HTTP Request Handlers  ////////////////////////////////////
void handleLed() {
  // http://xxx.xxx.xxx.xxx/led?val=1
  if(myWebServer.hasArg("val")) {
    int value = myWebServer.arg("val").toInt();
    digitalWrite(ledPin, value);
  }

  String reply = "LED is now ";
  reply += digitalRead(ledPin) ? "OFF" : "ON";
  myWebServer.send(200, "text/plain", reply);
}

void handleServo() {
  // Check if the required arguments are present
  if (myWebServer.hasArg("num") && myWebServer.hasArg("val")) {
    // Convert arguments to integers
    int servonum = myWebServer.arg("num").toInt();
    int value = myWebServer.arg("val").toInt();
    
    // Set the PWM value for the specified servo
    pwm.setPWM(servonum, 0, value);
    delay(500);
    
    // Create and send a reply
    String reply = "SERVO " + String(servonum) + " value " + String(value);
    myWebServer.send(200, "text/plain", reply);
  } else {
    // Send an error response if arguments are missing
    myWebServer.send(400, "text/plain", "Missing 'num' or 'val' parameter");
  }
}


void setup(){
  Serial.begin(115200);
  
  // PWM
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates
  delay(1);

  // FILESYSTEM INIT
  startFilesystem();

  // Try to connect to stored SSID, start AP if fails after timeout
  myWebServer.setAP("ESP_AP", "123456789");
  IPAddress myIP = myWebServer.startWiFi(15000);
  Serial.println("\n");

  // Add custom page handlers to webserver
  myWebServer.on("/led", HTTP_GET, handleLed);
  myWebServer.on("/servo", HTTP_GET, handleServo);
  
  // set /setup and /edit page authentication
  // myWebServer.setAuthentication("admin", "admin");

  // Enable ACE FS file web editor and add FS info callback function
  myWebServer.enableFsCodeEditor(getFsInfo);

  // Start webserver
  myWebServer.begin();
  Serial.print(F("ESP Web Server started on IP Address: "));
  Serial.println(myIP);
  Serial.println(F("Open /setup page to configure optional parameters"));
  Serial.println(F("Open /edit page to view and edit files"));

  pinMode(LED_BUILTIN, OUTPUT);
}


void loop() {
  // for (uint16_t pulselen = SERVOMIN; pulselen < SERVOMAX; pulselen++) {
  //   pwm.setPWM(0, 0, pulselen);
  // }
  // delay(500);
  // for (uint16_t pulselen = SERVOMAX; pulselen > SERVOMIN; pulselen--) {
  //   pwm.setPWM(0, 0, pulselen);
  // }
  // delay(500);

  myWebServer.run();
}