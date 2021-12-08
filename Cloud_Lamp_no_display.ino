//Pin   Function  ESP-8266 Pin
//TX  TXD   TXD
//RX  RXD   RXD
//A0  Analog input, max 3.3V input  A0
//D0  IO  GPIO16
//D1  IO, SCL   GPIO5
//D2  IO, SDA   GPIO4
//D3  IO, 10k Pull-up   GPIO0
//D4  IO, 10k Pull-up, BUILTIN_LED  GPIO2
//D5  IO, SCK   GPIO14
//D6  IO, MISO  GPIO12
//D7  IO, MOSI  GPIO13
//D8  IO, 10k Pull-down, SS   GPIO15
//G   Ground  GND
//5V  5V  -
//3V3   3.3V  3.3V
//RST   Reset   RST


#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager 
const char HTTP_CLOUD_SCRIPT[] PROGMEM = "<script>                                                            \
                                          function showVal(m,id){                                             \
                                                                var xhttp=new XMLHttpRequest;                 \
                                                                xhttp.open('POST','/?'+id+'='+m,true);        \ 
                                                                xhttp.send();                                 \
                                                                };                                            \
                                          setTimeout(function(){location.href=\"/\"}, 600000);                \
                                          </script>";
const char HTTP_CLOUD_MENU[] PROGMEM  = "<form action=\"/\"  method=\"POST\" id=\"on\">                        \
                                          <input type=\"hidden\" name=\"on\" />                               \
                                          <button type=\"submit\" >On    </button>                            \
                                          <!--range on-->                                                     \
                                         </form><br/>                                                         \
                                         <form action=\"/\" method=\"POST\">                                   \
                                          <input type=\"hidden\" name=\"off\" />                              \
                                          <button type=\"submit\" >Off   </button>                            \
                                         </form><br/>                                                         \
                                         <form action=\"/\" id=\"storm\"  method=\"POST\">                     \
                                          <input type=\"hidden\" name=\"storm\" />                            \
                                          <button type=\"submit\" >Storm </button><br/>                       \
                                          <!--range storm-->                                                  \
                                         </form><br/>                                                         \
                                         <form action=\"/\" id=\"sleep\"  method=\"POST\">                     \
                                          <input type=\"hidden\" name=\"sleep\" />                            \
                                          <button type=\"submit\"  value=\"sleep\">Sleep (30 min)</button>    \
                                          <!--range sleep-->                                                  \
                                         </form>                                                              \
                                         <p><!--message--></p>"; 
const char HTTP_CLOUD_RANGE[] PROGMEM = "<input type=\"range\" min=\"10\" max=\"1000\" step=\"10\" id=\"range\"         \
                                          onchange=\"showVal(this.value,document.getElementById('range').form.id)\" />  \
                                         <br/>";

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
const char* host = "CloudLamp";
typedef void (*Flash)(void);

//    FILE: lightning.pde
//  AUTHOR: Rob Tillaart
//    DATE: 2012-05-08
//
// PUPROSE: simulate lighning POC
//

int multiplier = 500;
int BETWEEN = 1000 + 359 * multiplier;
#define DURATION 50 //60 //43
#define TIMES 3

#define LEDPIN 13
int lightON = HIGH;
int lightOFF = LOW;

unsigned long lastTime1 = 0;
int waitTime1 = 0;
unsigned long lastTime2 = 0;
int waitTime2 = 0;
unsigned long lastTime3 = 0;
int waitTime3 = 0;
unsigned long lastTime4 = 0;
int waitTime4 = 0;
//unsigned long lastFadeTime1 = 0;
//int waitFadeTime1 = 0;

int mode = 2; //2 - on
//0 - off
//1 - storm
//3 - sleep

unsigned long sleepPreviousMillis = 0;
const unsigned long sleepTimeInterval = 1800000; //30 min
unsigned long sleepTimeEnd = 0;
const int buttonPin = 16;    // the number of the pushbutton pin

String getPage(){
  String page = "<html lang='en'><head><meta http-equiv='refresh' content='60' name='viewport' content='width=device-width, initial-scale=1'/>";
  page += "<link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css'><script src='https://ajax.googleapis.com/ajax/libs/jquery/3.1.1/jquery.min.js'></script><script src='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js'></script>";
  page += "<title>Cloud Lamp</title></head><body>";
  page += "<script>function showVal(m){var xhttp=new XMLHttpRequest; xhttp.open('POST','/storm',true); xhttp.send('storm='+m);}</script>";
  page += "<form action='/' name='on'     method='get'>   <button>On    </button></form><br/>";
  page += "<form action='/' name='off'    method='get'>   <button>Off   </button></form><br/>";
  page += "<form action='/' name='storm'  method='get'>   <button>Storm </button></form><br/>";
  page += "<input type='range min='10' max='1000' step='10' onchange='showVal(this.value)'><br/>";
  page += "<form action='/' name='sleep'  method='post'>  <button>Sleep (30 min)</button></form>";
  page += "</body></html>";
  return page;
}
void fadeout_from_state( int ledPin ) {
  for (int fadeValue = analogRead(ledPin) ; fadeValue >= 0; fadeValue -= 20) {
    // sets the value (range from 0 to 1023 // 254 for arduino modules):
    analogWrite(ledPin, fadeValue);
    // wait for 30 milliseconds to see the dimming effect
    delay(random(5, 20));
  }
  analogWrite(ledPin, 0);
  digitalWrite(ledPin, lightOFF);
}
void fadeout( int ledPin ) {
  for (int fadeValue = 1023 ; fadeValue >= 0; fadeValue -= 20) {
    // sets the value (range from 0 to 1023 // 254 for arduino modules):
    analogWrite(ledPin, fadeValue);
    // wait for 30 milliseconds to see the dimming effect
    delay(random(5, 20));
  }
  analogWrite(ledPin, 0);
  digitalWrite(ledPin, lightOFF);
}
void fadein( int ledPin ) {
  for (int fadeValue = 0 ; fadeValue <= 1000; fadeValue += 20) {
    // sets the value (range from 0 to 1023 // 254 for arduino modules):
    analogWrite(ledPin, fadeValue);
    // wait for 30 milliseconds to see the dimming effect
    delay(random(5, 20));
  }
  analogWrite(ledPin, 1023);
  digitalWrite(ledPin, lightON);
}

void sleep_light() {
  if ((millis() > sleepTimeEnd) || (millis() < sleepPreviousMillis)) // check timeout and mills rollover
  {
    //fadeout_from_state(D3);
    digitalWrite(D3,lightOFF);
  }
  sleepPreviousMillis = millis();
}

void clear_sleep() {
  sleepTimeEnd = 0;
  //fadeout_from_state(D3);
  //digitalWrite(D3,lightOFF);
}

void clear_storm_wait() {
  lastTime1 = millis();
  lastTime2 = millis();
  lastTime3 = millis();
  lastTime4 = millis();
  waitTime1 = 0;
  waitTime2 = 0;
  waitTime3 = 0;
  waitTime4 = 0;
}

void flash1() //-> couch side internal
{
  if (millis() - waitTime1 > lastTime1)  // time for a new flash
  {
    // adjust timing params
    lastTime1 += waitTime1;
    waitTime1 = random(BETWEEN);
    int times_local = random(1, TIMES);
    for (int i = 0; i < times_local; i++)
    {
      Serial.print("1");
      digitalWrite(D1, lightON);
      delay(20 + random(DURATION));
      if (times_local == 1) {
        fadeout(D1);
      }
      else {
        digitalWrite(D1, lightOFF);
        delay(20 + random(DURATION));
      }
    }
  }
}

void flash2()   //->lower
{
  if (millis() - waitTime2 > lastTime2)  // time for a new flash
  {
    // adjust timing params
    lastTime2 += waitTime2;
    waitTime2 = random(BETWEEN);
    int times_local = random(1, TIMES-1);
    for (int i = 0; i < times_local; i++)
    {
      Serial.print("2");
      digitalWrite(D2, lightON);
      delay(0 + random(DURATION));
      if (times_local == 1) {
//        fadeout(D2);
        digitalWrite(D2, lightOFF);
      }
      else {
        digitalWrite(D2, lightOFF);
        delay(20 + random(DURATION));
      }
    }
  }
}

void flash3() //->max power periferal
{
  if (millis() - waitTime3 > lastTime3)  // time for a new flash
  {
    // adjust timing params
    lastTime3 += waitTime3;
    waitTime3 = random(BETWEEN);
    int times_local = random(1, TIMES-1);
    for (int i = 0; i < times_local; i++)
    {
      Serial.print("3");
      digitalWrite(D3, lightON);
      delay(0 + random(DURATION));
      if (times_local == 1) {
//        fadeout(D3);
        digitalWrite(D3, lightOFF);
      }
      else {
        digitalWrite(D3, lightOFF);
        delay(20 + random(DURATION));
      }
    }
  }
}
void flash4() //TV side internal + upper small
{
  if (millis() - waitTime4 > lastTime4)  // time for a new flash
  {
    // adjust timing params
    lastTime4 += waitTime4;
    waitTime4 = random(BETWEEN);
    int times_local = random(1, TIMES);
    for (int i = 0; i < times_local; i++)
    {
      Serial.print("4");
      digitalWrite(D4, lightON);
      delay(20 + random(DURATION));
      if (times_local == 1) {
        fadeout(D4);
      }
      else {
        digitalWrite(D4, lightOFF);
        delay(20 + random(DURATION));
      }
    }
  }
}
void lightoff() {
  digitalWrite(D1, lightOFF);
  digitalWrite(D2, lightOFF);
  digitalWrite(D3, lightOFF);
  digitalWrite(D4, lightOFF);

  Serial.println("Light mode off");
  mode = 0;
}

void storm() {
  clear_storm_wait();
  if (server.arg("storm").toInt() >= 1) {
  //if (server.arg(0).toInt() >= 1) {
    multiplier = server.arg("storm").toInt();
  }
  BETWEEN = 1000 + 359 * multiplier;

  Serial.print("\nBETWEEN time is ");
  Serial.print(" = ");
  Serial.println(BETWEEN / 1000);
  
  //yield();
  Serial.println("Storm mode activated");
  mode = 1;
}

void lighton() {
  
  if (server.arg("on").toInt() >= 1) {
    analogWrite(D3, map(server.arg("on").toInt(), 0, 1000, 0, 255));
  }
  else {
    analogWrite(D3, 100);
  }
  digitalWrite(D2, lightON);

  Serial.println("Light mode on");
  mode = 2;
}
void sleep() {
  if (mode != 3) {
    sleepTimeEnd = millis() + sleepTimeInterval;
  }

  digitalWrite(D1, lightOFF);
  digitalWrite(D2, lightOFF);
  digitalWrite(D3, lightOFF);
  digitalWrite(D4, lightOFF);

  if (server.arg("sleep").toInt() >= 1) {
    analogWrite(D3, map(server.arg("sleep").toInt(), 0, 1000, 0, 255));
  }
  else {
    analogWrite(D3, 100);
  }
  
  //yield();
  Serial.println("Sleep mode");
  mode = 3;
}

void handleRoot() {
  
  String page = FPSTR(HTTP_HEADER);
  page.replace("{v}", "CloudLamp");
//  page += FPSTR(HTTP_SCRIPT);
  page += FPSTR(HTTP_CLOUD_SCRIPT);
  page += FPSTR(HTTP_STYLE);
  page += FPSTR(HTTP_HEADER_END);
  page += FPSTR(HTTP_CLOUD_MENU);
  page += FPSTR(HTTP_END);

  if ( server.hasArg("on") ) {
    page.replace("<!--range on-->", FPSTR(HTTP_CLOUD_RANGE));
    lighton( );
    Serial.println("Has argument ON");
  }
  else if ( server.hasArg("off") ) {
    lightoff( );
    Serial.println("Has argument OFF");
  }
  else if ( server.hasArg("storm") ) {
    page.replace("<!--range storm-->", FPSTR(HTTP_CLOUD_RANGE));
    storm( );  
       
//    String message = "POST form was:\n";
//    for (uint8_t i = 0; i < server.args(); i++) {
//      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
//    }
//     page.replace("<!--message-->", message);
     
    Serial.println("Has argument STORM");
  }
  else if ( server.hasArg("sleep") ) {
    page.replace("<!--range sleep-->", FPSTR(HTTP_CLOUD_RANGE));
    sleep( );
    Serial.println("Has argument SLEEP");
  }
  
//  Serial.println(server.args());
  
//  String arguments;
//  for (uint8_t i = 0; i < server.args(); i++) {
//    arguments += " " + server.argName(i) + ": " + server.arg(i) + "\n";
//  }
//  Serial.println(arguments);
//  Serial.println("Root page accessed");
    server.send ( 200, "text/html", page );
//  if (server.method() != HTTP_POST) {
//    server.send ( 200, "text/html", page );
//  } else {
//    String message = "POST form was:\n";
//    for (uint8_t i = 0; i < server.args(); i++) {
//      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
//    }
//    server.send(200, "text/plain", message);
//  }
//  server.send ( 200, "text/html", getPage() );
  
//  yield();
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not found");
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void setup()
{
  Serial.begin(115200);
  pinMode(buttonPin, INPUT);
  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
//  digitalWrite(D1, lightON);  //-> couch side internal
//  digitalWrite(D2, lightON);    //->lower
//  digitalWrite(D3, lightON);    //->max power periferal
//  digitalWrite(D4, lightON);  //TV side internal + upper small
//  fadein(D1);
//  analogWrite(D3, 100);         //->max power periferal
//  digitalWrite(D2, lightON);    //->lower
    lighton( );

  wifi_station_set_hostname("CloudLamp");
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;  // commented here because it is initiallized outside setup()
  //reset settings - for testing
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
//  if (!wifiManager.autoConnect("CloudLamp")) {
//    Serial.println("failed to connect and hit timeout");
//    //reset and try again, or maybe put it to deep sleep
//    ESP.reset();
//    delay(1000);
//    //if you get here you have connected to the WiFi
//    Serial.println("connected...yeey :)");
//  }

  wifiManager.autoConnect("CloudLamp");

  WiFi.hostname(host);
//  if (!MDNS.begin(host)) {
//    Serial.println("Error setting up MDNS responder!");
//  }

  MDNS.addService("http", "tcp", 80);
  httpUpdater.setup(&server);
  server.on( "/" , handleRoot );
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.print ( "HTTP server started on IP " );
  Serial.println ( WiFi.localIP().toString());
  Serial.printf ( "Update URL is http://%s.local/update in your browser\n", host  );
  Serial.print (F(__DATE__)) ;
  Serial.print (" ") ;
  Serial.println (F(__TIME__)) ;
}

Flash flashes[] = { flash1, flash2, flash3, flash4 };

void loop()
{

server.handleClient();  
yield();

  switch (mode) {
    case 0:             //0 - off
      clear_sleep();
      break;
    case 1:             //1 - storm
      clear_sleep();
      flashes[random(4)]();
      //      flash1();
      //      flash2();
      //      flash3();
      //      flash4();
      break;
    case 2:             //2 - on
        clear_sleep();
        clear_storm_wait();
      break;
    case 3:             //3 - sleep
      sleep_light();
      clear_storm_wait();
      break;
    default:
      // statements
      break;
  }

  // do other stuff here
//
//  while (Serial.available()) {
//
//    String serial_command = Serial.readString();
//    Serial.println (serial_command);
//    if (serial_command == "IP" || serial_command == "ip" )
//    {
//      Serial.println ( WiFi.localIP().toString());
//    }
//    else if (serial_command == "storm") {
//      storm();
//    }
//    else if (serial_command == "on") {
//      lighton();
//    }
//    else if (serial_command == "sleep") {
//      sleep();
//    }
//    else if (serial_command == "off") {
//      lightoff();
//    }
////    else if (serial_command == "resetwifi") {
////      WiFiManager wifiManager;
////      wifiManager.resetSettings();
////      Serial.println ( "Resetting WiFi settings..." );
////      ESP.restart();
////    }
//  }

  //  if (digitalRead(buttonPin) == HIGH)
  //  {
  //    Serial.println("Resetting WiFi settings");
  //    wifiManager.resetSettings();
  //    ESP.restart();
  //  }

}
