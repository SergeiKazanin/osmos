#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

const char *ssid = "MikroTik";
const char *password = "12345678";

ESP8266WebServer server(80);

float sp = 23,					  // set point
	pv = 0,						  // current temperature
	pv_last = 0,				  // prior temperature
	ierr = 0,					  // integral error
	dt = 0,						  // time between measurements
	op = 0;						  // PID controller output
unsigned long ts = 0, new_ts = 0; // timestamp

const char HTTP_HTML[] PROGMEM = "<!DOCTYPE html>\
<html>\
<head>\
	<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
	<script>\
		window.setInterval(\"update()\", 1000);\
		function update(){\
			var xhr=new XMLHttpRequest();\
			xhr.open(\"GET\", \"/temp\", true);\
			xhr.onreadystatechange = function () {\
				if (xhr.readyState != XMLHttpRequest.DONE || xhr.status != 200) return;\
				document.getElementById('temp').innerHTML=xhr.responseText;\
			};\
			xhr.send();\
		}\
	</script>\
</head>\
<body style=\"text-align:center\">\
    <h1>OpenTherm Thermostat</h1>\
    <font size=\"7\"><span id=\"temp\">{0}</span>&deg;</font>\
    <p>\
    <form method=\"post\">\
        Set to: <input type=\"text\" name=\"sp\" value=\"{1}\" style=\"width:50px\"><br/><br/>\
        <input type=\"submit\" style=\"width:100px\">\
    <form>\
    </p>\
</body>\
</html>";

void handleRoot()
{
	digitalWrite(LED_BUILTIN, 1);

	if (server.method() == HTTP_POST)
	{
		for (uint8_t i = 0; i < server.args(); i++)
		{
			if (server.argName(i) == "sp")
			{
				sp = server.arg(i).toFloat();
			}
		}
	}

	String page = FPSTR(HTTP_HTML);
	page.replace("{0}", String((int)sp));
	page.replace("{1}", String((int)sp));
	server.send(200, "text/html", page);
	digitalWrite(LED_BUILTIN, 0);
}

void handleGetTemp()
{
	digitalWrite(LED_BUILTIN, 0);
	server.send(200, "text/plain", String((int)sp));
	digitalWrite(LED_BUILTIN, 1);
}

void setup(void)
{
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, 1);
	Serial.begin(9600);
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);
	Serial.println("");

	// Wait for connection
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.print(".");
	}
	Serial.println("");
	Serial.print("Connected to ");
	Serial.println(ssid);
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());

	server.on("/", handleRoot);
	server.on("/temp", handleGetTemp);

	server.begin();
	Serial.println("HTTP server started");

	ts = millis();
}

void loop(void)
{
	new_ts = millis();
	if (new_ts - ts > 1000)
	{
    ts =new_ts;
		sp++;
	}
	server.handleClient(); // handle http requests
}
