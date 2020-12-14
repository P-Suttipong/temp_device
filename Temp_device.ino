#include <OneWire.h>
#include <DallasTemperature.h>
#include <EasyScheduler.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN 15
#define LED_COUNT 1
#define ONE_WIRE_BUS 4
#define ONE_WIRE_BUS_2 5
#define ONE_WIRE_BUS_3 21
OneWire oneWire(ONE_WIRE_BUS);
OneWire oneWire2(ONE_WIRE_BUS_2);
OneWire oneWire3(ONE_WIRE_BUS_3);
DallasTemperature sensors(&oneWire);
DallasTemperature sensors2(&oneWire2);
DallasTemperature sensors3(&oneWire3);
WiFiClient client;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);


const char* ssid = "AndamanTemp";
const char* password = "temp2020";
const uint16_t port = 2050;
const char * host = "dnscold-room.attg.cc";

Schedular readTempTask;
Schedular checkPowerTask;
Schedular sendDataTask;

String readString;
String MAC, temp_hex = "*";
long endTime = 0.0;
int sensitive = 185, offset = 2504, indCurrent;
char msg[50], power = 'N';
String formattedDate,dayStamp,timeStamp;
String date, times;
uint32_t datelong, timeslong;

//void sendData() {
//  snprintf (msg, 75, "%s,%s,%c", MAC.c_str(), temp_hex.c_str(), power);
//  WiFiClient client;
//
//  if (!client.connect(host, port)) {
//    Serial.println("Connection to host failed");
//    return;
//  }
//  Serial.println(msg);
//  client.print(msg);
//  delay(10);
//  client.stop();
//}



//***********************GET MAC**************************

void getMAC() {
  String mac = WiFi.macAddress();
  String arr;
  int i = 0;
  while (i < mac.length()) {
    if (mac.charAt(i) == ':') {
    } else {
      arr = arr + mac.charAt(i);
    }
    i++;
  }
  MAC = arr;
}


//***********************CHECKPOWER***********************

void checkPower() {
  int p = digitalRead(35);
  if(p == 1){
    power = 'P';
  }else{
    power = 'B';
  }
  Serial.print("POWER STATUS: ");
  Serial.println(digitalRead(35));
  
}

//***********************READ TEMP***********************

void readTemp() {
//  sensors.requestTemperatures();
//  float temp = sensors.getTempCByIndex(0);
//  int tempInt = int(temp * 100);
//  temp_hex = String(tempInt, HEX);
//  temp_hex.toUpperCase();
//  delay(1000);
  sensors.requestTemperatures();
  float temp1 = sensors.getTempCByIndex(0);
  delay(100);
  sensors2.requestTemperatures();
  float temp2 = sensors2.getTempCByIndex(0);
  delay(100);
  sensors3.requestTemperatures();
  float temp3 = sensors3.getTempCByIndex(0);
  delay(100);
  Serial.print("TEMP 1: ");
  Serial.print(temp1);
  Serial.print("  TEMP 2: ");
  Serial.print(temp2);
  Serial.print("  TEMP 3: ");
  Serial.println(temp3);
  float avg = 0;
  if(temp1 != -127.00 && temp2 != -127.00 && temp3 != -127.00){
    avg = (temp1 + temp2 + temp3) / 3;
  } else if(temp1 != -127.00 && temp2 != -127.00){
    avg = (temp1 + temp2) / 2; 
  } else if(temp1 != -127.00 && temp3 != -127.00){
    avg = (temp1 + temp3) / 2; 
  } else if(temp2 != -127.00 && temp3 != -127.00){
    avg = (temp2 + temp3) / 2; 
  } else if(temp1 != -127.00){
    avg = temp1;
  } else if(temp2 != -127.00){
    avg = temp2;
  } else if(temp3 != -127.00){
    avg = temp3;
  } else {
    avg = 0;
  }
  if(avg != 0){
    int tempInt = int(avg * 100);
    temp_hex = String(tempInt);
  } else {
    temp_hex = "*";
  }
}

//***********************GET DATE***********************

void getDate() {
  formattedDate = timeClient.getFormattedDate();
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  timeStamp = formattedDate.substring(splitT + 1, formattedDate.length() - 1);
  int i = 0, j = 0;
  String stackDay, stackTime;
  
  while (i < dayStamp.length()) {
    if (dayStamp.charAt(i) == '-') {
    } else {
      stackDay = stackDay + dayStamp.charAt(i);
    }
    i++;
  }
  dayStamp = stackDay.substring(2,8);
  
  while (j < timeStamp.length()) {
    if (timeStamp.charAt(j) == ':') {
    } else {
      stackTime = stackTime + timeStamp.charAt(j);
    }
    j++;
  }
  timeStamp = stackTime.substring(0,6);
  
//  date = dayStamp + timeStamp;
  datelong = dayStamp.toInt();
  timeslong = timeStamp.toInt();
  times = String(timeslong, HEX);
  date = String(datelong, HEX);
  date.toUpperCase();
  times.toUpperCase();
}

void colorWipe(uint32_t color, int wait) {
  for(int i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, color);         
    strip.show();                                                  
  }
}

//***********************SETUP***********************

void setup(void) {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println("TEMPERATURES DEVICE");

  while (MAC.length() < 1) {
    getMAC();
  }

  pinMode(35, INPUT);

  Serial.println();
  Serial.print("WiFi CONNECTING TO :  ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi CONNECTED");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  timeClient.begin();
  timeClient.setTimeOffset(7*3600);

  Serial.println();
  Serial.print("CONNECTING TO :  ");
  Serial.print(host);
  Serial.print(":");
  Serial.println(port);
  while (!client.connect(host, port)) {
    delay(500);
    Serial.print("*");
  }
  Serial.println("SOCKET CONNECTED");

  sensors.begin();

  strip.begin();
  strip.show();
  strip.setBrightness(50);

  readTempTask.start();
  checkPowerTask.start();
  sendDataTask.start();
}

//***********************LOOP***********************

void loop(void) {
  readTempTask.check(readTemp, 3000);
  checkPowerTask.check(checkPower, 1000);

  if(WiFi.status() != WL_CONNECTED){
    WiFi.begin(ssid, password);
  }

  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  getDate();

  if (!client.connected()) {
    while (!client.connect(host, port)) {
      Serial.print("*");
    }
  } else {
    long nowtime = millis();
    if (nowtime - endTime > 60000) {
      endTime = nowtime;
      snprintf (msg, 75, "%s,%s,%s,%s,%c", times.c_str() ,date.c_str(), MAC.c_str(), temp_hex.c_str(), power);
      Serial.println(msg);
      client.println(msg);
    }
  }

  if (client.available()) {
    char c = client.read();
    if (readString.length() < 100) {
      readString += c;
    }
    Serial.print(readString);
    readString = "";
  }

  if(power == 'P'){
    colorWipe(strip.Color(  0, 255,   0), 50);
  }else{
    colorWipe(strip.Color(  255, 0,   0), 50);
  }
}
