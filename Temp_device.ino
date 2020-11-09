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
const uint16_t port = 77;
const char * host = "192.168.1.122";

Schedular readTempTask;
Schedular checkPowerTask;
Schedular sendDataTask;

String readString;
String MAC, temp_hex = "*";
long endTime = 0.0;
int sensitive = 185, offset = 2504, indCurrent;
char msg[50], power = 'N';
String formattedDate,dayStamp,timeStamp;
String date;

void sendData() {
  snprintf (msg, 75, "%s,%s,%c", MAC.c_str(), temp_hex.c_str(), power);
  WiFiClient client;

  if (!client.connect(host, port)) {
    Serial.println("Connection to host failed");
    return;
  }
  Serial.println(msg);
  client.print(msg);
  delay(10);
  client.stop();
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

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

double getCurrentAvg() {
  int count = 200;
  double sum = 0;
  for (int i = 0; i < count; i++) {
    sum += getCurrent();
  }
  double val = sum / count;
  return val;
}

double getCurrent() {
  int raw = analogRead(35);
  double volt = (raw / 1024.0) * 5000;
  double current = (volt - offset) / sensitive;
  return current;
}

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

void checkPower() {
  int nowCurrent = int(getCurrentAvg());
  if (nowCurrent < indCurrent) {
    power = 'B';
  } else {
    power = 'P';
  }
}

void readTemp() {
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);
  int tempInt = int(temp * 100);
  temp_hex = String(tempInt, HEX);
  temp_hex.toUpperCase();
  delay(1000);
}

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
  int dayInt = dayStamp.toInt();
  dayStamp = String(dayInt, HEX);
  
  while (j < timeStamp.length()) {
    if (timeStamp.charAt(j) == ':') {
    } else {
      stackTime = stackTime + timeStamp.charAt(j);
    }
    j++;
  }
  timeStamp = stackTime.substring(0,4);
  int timeInt = timeStamp.toInt();
  timeStamp = String(timeInt, HEX);
  date = dayStamp + timeStamp;
  date.toUpperCase();
}

void setup(void) {
  Serial.begin(115200);
  Serial.println("TEMPERATURES DEVICE");

  while (MAC.length() < 1) {
    getMAC();
  }

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  timeClient.begin();
  timeClient.setTimeOffset(7*3600);

  while (!client.connect(host, port)) {
    delay(500);
    Serial.print("*");
  }

  sensors.begin();

  indCurrent = initPower();
  Serial.println(indCurrent);

  readTempTask.start();
  checkPowerTask.start();
  sendDataTask.start();
}

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
      snprintf (msg, 75, "%s,%s,%s,%c", date.c_str(), MAC.c_str(), temp_hex.c_str(), power);
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
