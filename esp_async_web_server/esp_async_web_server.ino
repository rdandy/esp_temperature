/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com  
*********/

// Import required libraries
#ifdef ESP32
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#else
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#endif
#include <ctype.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// include TM1637 lib
#include <TM1637.h>

// Relay pin
int RELAY = 21;

// Temperature data wire is connected to GPIO 4
int ONE_WIRE_BUS = 4;
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

// TM1637 7 segment display
int PIN_DIO = 16;
int PIN_CLK = 17;
TM1637 tm(PIN_CLK, PIN_DIO);
// SevenSegmentTM1637    display(PIN_CLK, PIN_DIO);

// Variables to store temperature values
String temperatureF = "";
String temperatureC = "";
String host = "";
float tempC = 0.0;
float tempF = 0.0;
float high_temp = 26.0;
float low_temp = 22.0;
String err_msg_h = "";
String err_msg_l = "";
bool have_wifi = false;
int relay_output = LOW;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 2000;
unsigned long lastCheckWifiTime = 0;
unsigned long timerCheckWifiDelay = 60000;
unsigned long wifi_time_limit = 20000;

// Replace with your network credentials
const char *ssid = "ihavewateryounot";
const char *password = "rcazj0317";
String local_ip = "";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

String readDSTemperatureC() {
  // Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  if (tempC == -127.00) {
    Serial.println("Failed to read from DS18B20 sensor");
    return "--";
  } else {
    Serial.print("Temperature Celsius: ");
    Serial.println(tempC);
  }
  return String(tempC);
}

String readDSTemperatureF() {
  // Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
  sensors.requestTemperatures();
  float tempF = sensors.getTempFByIndex(0);

  if (int(tempF) == -196) {
    Serial.println("Failed to read from DS18B20 sensor");
    return "--";
  } else {
    Serial.print("Temperature Fahrenheit: ");
    Serial.println(tempF);
  }
  return String(tempF);
}

const char styles_css[] PROGMEM = R"rawliteral(
html {
  font-family: Arial;
  display: inline-block;
  margin: 0px auto;
  text-align: center;
}
h2 { font-size: 3.0rem; }
p { font-size: 3.0rem; }
.units { font-size: 1.2rem; }
.ds-labels{
  font-size: 1.5rem;
  vertical-align:middle;
  padding-bottom: 15px;
}
form.temp_form input[type=number]
{
  padding: 15px 32px;
  font-size: 34px;
  width: 6.5rem;
  height: 2.1rem;
  line-height: 1.7;
  display: inline-block;
  padding: 4px;
  margin: 0;
  border: 1px solid #eee;
}
form.temp_form button, .button {
  background-color: #008CBA; /* Green */
  border: none;
  color: white;
  padding: 15px 32px;
  text-align: center;
  text-decoration: none;
  border-radius: 8px;
  display: inline-block;
  font-size: 20px;
  cursor: pointer;
}
form.temp_form input[type=number]:focus {
  outline: 0;
}
.errmsg {
  font-size: 1.3rem;
  font-color: #A00;
}
)rawliteral";

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP DS18B20 Temperature Server</title>
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="stylesheet" href="/styles.css">
  <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.6.0/jquery.min.js"></script>
</head>
<body>
  <h2>ESP DS18B20 Server</h2>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="ds-labels">Temperature Celsius</span> 
    <span id="temperaturec">%TEMPERATUREC%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="ds-labels">Temperature Fahrenheit</span>
    <span id="temperaturef">%TEMPERATUREF%</span>
    <sup class="units">&deg;F</sup>
  </p>

  <p>
    Turn Fan On When Temperature is above: %HIGHTEMPERATURE%
    <br>
    Turn Fan Off When Temperature is below: %LOWTEMPERATURE%
    <br>
    <a href="/update_temperature" class="button">Settings</a>
  </p>
</body>
<script>
setInterval(function () {
  $.ajax({
    url: "/states",
    type: "GET",
    dataType: "json",
    success: function(data) {
      $("#temperaturec").innerHTML = data["temperaturec"];
      $("#temperaturef").innerHTML = data["temperaturef"];
    },
    error: function() {
      alert("Internet ERROR!!!");
    }
  });
}, 10000);
</script>
</html>)rawliteral";

const char update_temperature[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP DS18B20 Temperature Server - Update Temperature</title>
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="stylesheet" href="/styles.css">
</head>
<body>
  <h2>ESP DS18B20 Server - Edit Temperature</h2>
  <p>
    <a href="/">Home</a>
  </p>
  <p>
    <form action="/update_temperature" method="post" class="temp_form">
      <p>
        Turn Fan On When Temperature is above:
        <input type="number" value="%HIGHTEMPERATURE%" name="high_temp" min="0" max="40.0" step="0.1">
      </p>
      <span class="errmsg">%ERRMESSAGEH%</span>
      <p>
        Turn Fan Off When Temperature is below:
        <input type="number" value="%LOWTEMPERATURE%" name="low_temp" min="0" max="40.0" step="0.1">
      </p>
      <span class="errmsg">%ERRMESSAGEL%</span>
      
      <button type="submit">Submit</button>
    </form>
  </p>
</body>
</html>)rawliteral";

const char states[] PROGMEM = R"rawliteral(
{
  "temperaturec": %TEMPERATUREC%,
  "temperaturef": %TEMPERATUREF%,
  "temperature_high": %HIGHTEMPERATURE%,  
  "temperature_low": %LOWTEMPERATURE%,
  "have_wifi": %have_wifi%,
  "relay_output": %relay_output%
}
)rawliteral";

const char not_found[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP DS18B20 Temperature Server - Not Found</title>
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="stylesheet" href="/styles.css">
</head>
<body>
  <h2>ESP DS18B20 Server - Not Found</h2>
  <p>
    <a href="/">Home</a>
  </p>
</body>
</html>)rawliteral";

// Replaces placeholder with DS18B20 values
String processor(const String &var) {
  //Serial.println(var);
  if (var == "TEMPERATUREC") {
    return temperatureC;
  } else if (var == "TEMPERATUREF") {
    return temperatureF;
  } else if (var == "HIGHTEMPERATURE") {
    return String(high_temp);
  } else if (var == "LOWTEMPERATURE") {
    return String(low_temp);
  } else if (var == "relay_output") {
    return String(relay_output);
  } else if (var == "ERRMESSAGEH") {
    return (String)err_msg_h;
  } else if (var == "ERRMESSAGEL") {
    return (String)err_msg_l;
  } else if (var == "have_wifi") {
    if (have_wifi) {
      return "true";
    }
    return "false";
  }
  return String();
}


void set_relay() {
  Serial.println((String) "Relay is set to " + String(relay_output));
  digitalWrite(RELAY, relay_output);
}


void check_wifi() {
  if (lastCheckWifiTime == 0){
    Serial.print((String) "Connect to WiFi .." + ssid + " / " + password);
    WiFi.begin(ssid, password);
  }

  if ((lastCheckWifiTime > 0) and (millis() - lastCheckWifiTime) < timerCheckWifiDelay) {
    return;
  }

  // Test Wi-Fi status
  if (WiFi.status() == WL_CONNECTED) {
    lastCheckWifiTime = millis();
    have_wifi = true;
    return;
  }

  have_wifi = false;
  Serial.print((String) "Checking WiFi .." + ssid + " / " + password);
  while (WiFi.status() != WL_CONNECTED) {
    if ((millis() - lastCheckWifiTime) < wifi_time_limit) {
      delay(500);
      Serial.print(">");
    } else {
      break;
    }
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    have_wifi = true;
    Serial.println("WiFi Connected!");
  } else {
    have_wifi = false;
    Serial.print((String) "... Can not connect to " + ssid + ".");
    ESP.restart();
  }
  lastCheckWifiTime = millis();
}


void setup() {
  // Serial port for debugging purposes

  Serial.begin(9600);
  Serial.println();
  Serial.println((String) "SSID/Password: " + ssid + " / " + password);

  pinMode(RELAY, OUTPUT);
  set_relay();

  // start display
  tm.begin();
  tm.setBrightness(3);
  tm.display("INIT");
  delay(1000);
  // Start up the DS18B20 library
  sensors.begin();
  temperatureC = readDSTemperatureC();
  temperatureF = readDSTemperatureF();
  if (temperatureC == "--") {
    tm.clearScreen();
    tm.display("ERR");
  } else {
    tm.display(temperatureC.toFloat());
  }

  check_wifi();

  // Connect to Wi-Fi
  // WiFi.begin(ssid, password);
  // Serial.print("Connecting to WiFi ..");
  // lastTime = millis();
  // while (WiFi.status() != WL_CONNECTED) {
  //   if ((millis() - lastTime) <= wifi_time_limit) {
  //     delay(500);
  //     Serial.print(".");
  //   } else {
  //     break;
  //   }
  // }
  // Serial.println();
  // if (WiFi.status() == WL_CONNECTED) {
  //   Serial.println("WiFi Connected!");
  //   have_wifi = true;
  // } else {
  //   Serial.print((String) "... Can not connect to " + ssid + ".");
  // }

  lastTime = millis();
  // Print ESP Local IP Address
  local_ip = WiFi.localIP().toString();
  Serial.println((String) "local_ip: " + local_ip + " and preparing web serverice...");
  // Route for root / web page
  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/css", styles_css);
  });
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html, processor);
  });
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/html", not_found);
  });
  server.on("/temperaturec", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", temperatureC.c_str());
  });
  server.on("/temperaturef", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", temperatureF.c_str());
  });
  server.on("/states", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/json", states, processor);
  });
  server.on("/update_temperature", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", update_temperature, processor);
  });
  server.on("/update_temperature", HTTP_POST, [](AsyncWebServerRequest *request) {
    err_msg_h = "";
    err_msg_l = "";
    if (request->hasParam("high_temp", true)) {
      AsyncWebParameter *p_h = request->getParam("high_temp", true);
      // String v = p->value().c_str();
      float _t = p_h->value().toFloat();
      if (_t == 0.0) {
        Serial.println((String) "Input is not a float value: " + _t);
        err_msg_h = "Input is not a float value.";
        request->redirect("/update_temperature");
      } else {
        high_temp = _t;
        Serial.println(_t);
      }
    }
    if (request->hasParam("low_temp", true)) {
      AsyncWebParameter *p_l = request->getParam("low_temp", true);
      // String v = p->value().c_str();
      float _t = p_l->value().toFloat();
      if (_t == 0.0) {
        Serial.println((String) "Input is not a float value: " + _t);
        err_msg_l = "Input is not a float value.";
        request->redirect("/update_temperature");
      } else {
        low_temp = _t;
        Serial.println(_t);
      }
    }
    request->redirect("/");
  });
  // Start server
  server.begin();
}


void loop() {
  if ((millis() - lastTime) > timerDelay) {
    temperatureC = readDSTemperatureC();
    temperatureF = readDSTemperatureF();
    Serial.println((String) "local_ip: " + local_ip);

    if (temperatureC == "--") {
      Serial.println("TemperatureC Sensor error!");
      tm.clearScreen();
      tm.display("ERR");
    } else {
      float cc = temperatureC.toFloat();
      tm.display(cc);
      if (cc >= high_temp) {
        Serial.println("Above high");
        // digitalWrite(RELAY, HIGH);
        relay_output = HIGH;
        set_relay();

      } else if (cc <= low_temp) {
        Serial.println("Below low");
        // digitalWrite(RELAY, LOW);
        relay_output = LOW;
        set_relay();
      } else {
        Serial.println("Temperature is OK!");
      }
    }
    lastTime = millis();
  }
  check_wifi();
}