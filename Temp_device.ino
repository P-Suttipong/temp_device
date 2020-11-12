#include <OneWire.h>
#include <DallasTemperature.h>
#include <EasyScheduler.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <NTPClient.h>

#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
WiFiClient client;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

const char* ssid = "AT-Developer-24G";
const char* password = "ATWifi2020";
const uint16_t port = 2050;
const char * host = "103.246.17.19";

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

//********************GET CURRENT AVG**********************

double getCurrentAvg() {
  int count = 200;
  double sum = 0;
  for (int i = 0; i < count; i++) {
    sum += getCurrent();
  }
  double val = sum / count;
  return val;
}

//***********************GET CURRENT***********************

double getCurrent() {
  int raw = analogRead(35);
  double volt = (raw / 1024.0) * 5000;
  double current = (volt - offset) / sensitive;
  return current;
}

//***********************INITPOWER***********************

int initPower() {
  double tmp = 9999;
  for (int i = 0; i < 100; i++) {
    double current = getCurrentAvg();
    if (current < tmp) {
      tmp = current;
    }
    delay(100);
  }
  int ind = int(tmp);
  return ind - 1;
}

//***********************CHECKPOWER***********************

void checkPower() {
  int nowCurrent = int(getCurrentAvg());
  if (nowCurrent < indCurrent) {
    power = 'B';
  } else {
    power = 'P';
  }
}

//***********************READ TEMP***********************

void readTemp() {
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);
  int tempInt = int(temp * 100);
  temp_hex = String(tempInt, HEX);
  temp_hex.toUpperCase();
  delay(1000);
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

//***********************SETUP***********************

void setup(void) {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println("TEMPERATURES DEVICE");
  
  Serial.println("INITIAL POWER");
  indCurrent = initPower();

  while (MAC.length() < 1) {
    getMAC();
  }

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

  readTempTask.start();
  checkPowerTask.start();
  sendDataTask.start();
}

//***********************LOOP***********************

void loop(void) {
  readTempTask.check(readTemp, 3000);
  checkPowerTask.check(checkPower, 2000);

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
    if (nowtime - endTime > 10000) {
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

}
