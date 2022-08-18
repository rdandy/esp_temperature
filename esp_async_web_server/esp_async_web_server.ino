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

// icclude TM1637 lib
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
// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 2000;
unsigned long wifi_time_limit = 20000;
bool have_wifi = false;

// Replace with your network credentials
const char *ssid = "ihavewateryounot";
const char *password = "rcazj0317";

// const char *ssid = "FLHShowroom";
// const char *password = "Showroom2021";

float high_temp = 26.0;
float low_temp = 22.0;

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
})rawliteral";

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP DS18B20 Temperature Server</title>
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="stylesheet" href="/styles.css">
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
    Turn Fan On When Temperature is above: %HIGHTEMPERATURE% <a href="update_high">Edit</a>
  </p>
  <p>
    Turn Fan Off When Temperature is below: %LOWTEMPERATURE% <a href="update_low">Edit</a>
  </p>
</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperaturec").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperaturec", true);
  xhttp.send();
}, 10000) ;
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperaturef").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperaturef", true);
  xhttp.send();
}, 10000) ;
</script>
</html>)rawliteral";

const char update_high[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP DS18B20 Temperature Server - Update High</title>
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="stylesheet" href="/styles.css">
</head>
<body>
  <h2>ESP DS18B20 Server - Edit High Temperature</h2>
  <p>
    <a href="/">Home</a>
  </p>
  <p>
    <form action="/update_high" method="post">
      Turn Fan On When Temperature is above: <input type="number" value="%HIGHTEMPERATURE%" name="high_temp" min="0" max="40" step="0.1"> <button type="submit">Submit</button>
    </form>
  </p>
</body>
</html>)rawliteral";

const char update_low[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP DS18B20 Temperature Server - Update Low</title>
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="stylesheet" href="/styles.css">
</head>
<body>
  <h2>ESP DS18B20 Server - Edit Low Temperature</h2>
  <p>
    <a href="/">Home</a>
  </p>
  <p>
    <form action="/update_low" method="post">
      Turn Fan Off When Temperature is below: <input type="number" value="%LOWTEMPERATURE%" name="low_temp" min="0" max="40" step="0.1"> <button type="submit">Submit</button>
    </form>
  </p>
</body>
</html>)rawliteral";

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
    return (String)high_temp;
  } else if (var == "LOWTEMPERATURE") {
    return (String)low_temp;
  }

  return String();
}


void setup() {
  // Serial port for debugging purposes

  Serial.begin(115200);
  Serial.println();
  Serial.println((String) "SSID/Password: " + ssid + " / " + password);

  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, LOW);

  // start display
  tm.begin();
  tm.setBrightness(4);
  tm.display("INIT");

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

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  lastTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if ((millis() - lastTime) <= wifi_time_limit) {
      delay(500);
      Serial.print(".");
    } else {
      break;
    }
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi Connected!");
    have_wifi = true;
  } else {
    Serial.print((String) "... Can not connect to " + ssid + ".");
  }

  lastTime = millis();
  if (have_wifi) {
    // Print ESP Local IP Address
    String local_ip = WiFi.localIP().toString().c_str();
    Serial.print("local_ip: " + local_ip + " and preparing web serverice...");
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
    server.on("/update_high", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", update_high, processor);
    });
    server.on("/update_high", HTTP_POST, [](AsyncWebServerRequest *request) {
      if (request->hasParam("high_temp", true)) {
        AsyncWebParameter *p = request->getParam("high_temp", true);
        String v = p->value().c_str();
        Serial.println(v);
        // if (isDigit(v)) {
        //   Serial.println("is digit");
        // } else {
        //   Serial.println("is not digit");
        // }


      } else {
        Serial.println("No data");
      }
      request->redirect("/");
    });
    server.on("/update_low", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", update_low, processor);
    });
    server.on("/update_low", HTTP_POST, [](AsyncWebServerRequest *request) {
      if (request->hasParam("low_temp", true)) {
        AsyncWebParameter *p = request->getParam("low_temp", true);
        Serial.println(p->value().c_str());
      } else {
        Serial.println("No data");
      }
      request->redirect("/");
    });
    // Start server
    server.begin();
  }
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    temperatureC = readDSTemperatureC();
    temperatureF = readDSTemperatureF();

    if (temperatureC == "--") {
      Serial.println("TemperatureC Sensor error!");
      tm.clearScreen();
      tm.display("ERR");
    } else {
      float cc = temperatureC.toFloat();
      tm.display(cc);
      if (cc >= high_temp) {
        Serial.println("Above high");
        digitalWrite(RELAY, HIGH);
      } else if (cc <= low_temp) {
        Serial.println("Below low");
        digitalWrite(RELAY, LOW);
      } else {
        Serial.println("Temperature is OK!");
      }
    }
    lastTime = millis();
  }
}
