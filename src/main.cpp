#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

const char *ssid = "MikroTik";
const char *password = "12345678";

ESP8266WebServer server(80);

int filterOnTime = 10;
int filterOnFlush = 3;
int flushingInterval = 6;

boolean filterOn = false;
int filterOnCount = 0, flag = 0;
unsigned long ts = 0, tsFlush = 0, new_ts = 0, hourMils = 3600000, minMils = 60000, tsButton = 0, buttonMils = 50, tsSec = 0;
unsigned long upTime = 0, totalTimeOnFilter = 0;

int EEPROM_int_read(int addr)
{
  byte raw[2];
  for (byte i = 0; i < 2; i++)
    raw[i] = EEPROM.read(addr + i);
  int &num = (int &)raw;
  return num;
}

void EEPROM_int_write(int addr, int num)
{
  byte raw[2];
  (int &)raw = num;
  for (byte i = 0; i < 2; i++)
  {
    EEPROM.write(addr + i, raw[i]);
  }
}
String getReadableTime(unsigned long time)
{
  String readableTime;
  unsigned long minutes;
  unsigned long hours;
  unsigned long days;

  minutes = time / 60;
  hours = minutes / 60;
  days = hours / 24;
  time %= 60;
  minutes %= 60;
  hours %= 24;

  if (days > 0)
  {
    readableTime = String(days) + " days ";
  }
  if (hours > 0)
  {
    readableTime += String(hours) + ":";
  }
  if (minutes < 10)
  {
    readableTime += "0";
  }
  readableTime += String(minutes) + ":";
  if (time < 10)
  {
    readableTime += "0";
  }
  readableTime += String(time);
  return readableTime;
}

const char HTTP_HTML[] PROGMEM = "<!DOCTYPE html>\
<html>\
<head>\
	<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
  <style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\
    .main{margin-top: 50px; display: flex; flex-direction: column; justify-content: center; align-items: center; gap: 30px}\
    .form{display: flex; flex-direction: column; justify-content: center; align-items: start; gap: 10px}\
    .button {width: 100px;background-color: #1abc9c;border: none;color: white; text-decoration: none;font-size: 25px; cursor: pointer;border-radius: 8px;}\
    .button-send {width: 100px;background-color: #1abc9c;border: none; color: white; text-decoration: none;font-size: 25px; cursor: pointer;border-radius: 8px;}\
    .button:active {background-color: #16a085;}\
  </style>\
	<script>\
		window.setInterval(\"update()\", 2000);\
		function update(){\
			var xhr=new XMLHttpRequest();\
			xhr.open(\"GET\", \"/filterOnCount\", true);\
			xhr.onreadystatechange = function () {\
				if (xhr.readyState != XMLHttpRequest.DONE || xhr.status != 200) return;\
				document.getElementById('filterOnCount').innerHTML=xhr.responseText;\
			};\
			xhr.send();\
		}\
	</script>\
</head>\
<body>\
    <div class=\"main\">\
      <font size=\"5\">Time to end of filtration: <span id=\"filterOnCount\">{0}</span> min</font> \
      <a class=\"button\" href=\"/filterOnButton\">On</a>\
      <a class=\"button\" href=\"/filterOffButton\">Off</a>\
      <form class=\"form\" method=\"post\">\
        <div>\
          <span>Filtration time in min: </span>\
          <input type=\"text\" name=\"filterOnTime\" value=\"{1}\" style=\"width:50px\">\
        </div>\
        <div>\
          <span>Flushing time in min: </span>\
          <input type=\"text\" name=\"filterOnFlush\" value=\"{2}\" style=\"width:50px\">\
        </div>\
        <div>\
          <span>Flushing interval in hours: </span>\
          <input type=\"text\" name=\"flushingInterval\" value=\"{3}\" style=\"width:50px\">\
        </div>\
        <span>Wifi signal strength: {4} </span>\
        <span>Up time: {5} </span>\
        <span>Total filtration time : {6} </span>\
        <input class=\"button-send\" type=\"submit\" value=\"Send\">\
      </form>\
    </div>\
</body>\
</html>";

void handleRoot()
{
  boolean change = false;
  if (server.method() == HTTP_POST)
  {
    for (uint8_t i = 0; i < server.args(); i++)
    {
      if (server.argName(i) == "filterOnTime")
      {
        if (filterOnTime != server.arg(i).toInt())
        {
          filterOnTime = server.arg(i).toInt();
          change = true;
        }
      }
      if (server.argName(i) == "filterOnFlush")
      {
        if (filterOnFlush != server.arg(i).toInt())
        {
          filterOnFlush = server.arg(i).toInt();
          change = true;
        }
      }
      if (server.argName(i) == "flushingInterval")
      {
        if (flushingInterval != server.arg(i).toInt())
        {
          flushingInterval = server.arg(i).toInt();
          change = true;
        }
      }
    }
  }
  if (change == true)
  {
    change = false;
    EEPROM_int_write(0, filterOnTime);
    EEPROM_int_write(2, filterOnFlush);
    EEPROM_int_write(4, flushingInterval);
    EEPROM.commit();
  }
  String page = FPSTR(HTTP_HTML);
  page.replace("{0}", String(filterOnCount));
  page.replace("{1}", String(filterOnTime));
  page.replace("{2}", String(filterOnFlush));
  page.replace("{3}", String(flushingInterval));
  page.replace("{4}", String(WiFi.RSSI()));
  page.replace("{5}", String(getReadableTime(upTime)));
  page.replace("{6}", String(totalTimeOnFilter));
  server.send(200, "text/html", page);
}
void onFilter(int onTime)
{
  filterOn = true;
  tsFlush = millis();
  ts = millis();
  filterOnCount = onTime;
  totalTimeOnFilter += onTime;
  digitalWrite(LED_BUILTIN, 0);
  digitalWrite(D6, 1);
}
void offFilter()
{
  filterOn = false;
  filterOnCount = 0;
  digitalWrite(LED_BUILTIN, 1);
  digitalWrite(D6, 0);
}

void handleFilterOnCount()
{
  server.send(200, "text/plain", String(filterOnCount));
}
void handleFilterOnButton()
{
  onFilter(filterOnTime);
  server.sendHeader("Location", String("/"), true);
  server.send(302, "text/plain", "");
}
void handleFilterOffButton()
{
  offFilter();
  server.sendHeader("Location", String("/"), true);
  server.send(302, "text/plain", "");
}

void setup(void)
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(D6, OUTPUT);
  pinMode(D7, INPUT_PULLUP);
  digitalWrite(LED_BUILTIN, 1);
  digitalWrite(D6, 0);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  server.on("/", handleRoot);
  server.on("/filterOnCount", handleFilterOnCount);
  server.on("/filterOnButton", handleFilterOnButton);
  server.on("/filterOffButton", handleFilterOffButton);
  server.begin();
  ts = millis();
  tsFlush = millis();
  tsButton = millis();
  EEPROM.begin(10);
  filterOnTime = EEPROM_int_read(0);
  filterOnFlush = EEPROM_int_read(2);
  flushingInterval = EEPROM_int_read(4);
}

void loop(void)
{
  server.handleClient();

  new_ts = millis();
  if (new_ts - ts >= minMils)
  {
    ts = new_ts;
    if (filterOn == true)
    {
      filterOnCount--;
    }
    if (filterOnCount == 0)
    {
      offFilter();
    }
  }

  if (new_ts - tsFlush >= flushingInterval * hourMils)
  {
    tsFlush = new_ts;
    onFilter(filterOnFlush);
  }

  if (new_ts - tsButton >= buttonMils)
  {
    tsButton = new_ts;
    if (digitalRead(D7) == LOW && flag == 0)
    {
      flag = 1;
      if (filterOn == false)
      {
        onFilter(filterOnTime);
      }
      else
      {
        offFilter();
      }
    }
    if (digitalRead(D7) == HIGH)
    {
      flag = 0;
    }
    if (WiFi.status() != WL_CONNECTED && filterOn == false)
    {
      digitalWrite(LED_BUILTIN, 0);
      delay(1);
      digitalWrite(LED_BUILTIN, 1);
    }
  }

  if (new_ts - tsSec >= 1000)
  {
    tsSec = new_ts;
    upTime++;
  }
}
