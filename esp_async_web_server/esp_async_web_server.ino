/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com  
*********/

// Import required libraries
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ESPAsyncWebServer.h>
#include <ctype.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <HTTPClient.h>

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
float low_temp = 23.0;
String err_msg_h = "";
String err_msg_l = "";
boolean have_wifi = false;
int relay_output = LOW;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 2000;
unsigned long lastCheckWifiTime = 0;
unsigned long timerCheckWifiDelay = 60000;
unsigned long wifi_time_limit = 30000;
unsigned long last_send_cloud_time = 0;
unsigned long last_send_line_time = 0;
unsigned long last_send_ifttt_time = 0;
unsigned long send_cloud_time_limit = 20000;   // 免費的 thingspeak 間隔要15秒

int abnormalCount = 0; //異常計次
boolean Alerted = false; //通知是否已發

// Replace with your network credentials
// const char *ssid = "ihavewateryounot";
// const char *password = "rcazj0317";
// const char *ssid = "andyiphone";
// const char *password = "iloveandy";

const char *ssid = "ihavewateryounot";
const char *password = "rcazj0317";

String local_ip = "";

// Line
WiFiClientSecure client;

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
      $("#temperaturec").html(data["temperaturec"]);
      $("#temperaturef").html(data["temperaturef"]);
    },
    error: function() {
      console.log("Internet ERROR!!!");
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
  // Serial.println((String) "Relay is set to " + String(relay_output));
  Serial.println("Relay is set to " + String(relay_output));
  digitalWrite(RELAY, relay_output);
}


void check_wifi() {
  if (lastCheckWifiTime == 0) {
    Serial.println((String) "Connect to WiFi .." + ssid + " / " + password);
    // WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(500);
    WiFi.begin(ssid, password);
  }

  if ((lastCheckWifiTime > 0) and ((millis() - lastCheckWifiTime) < timerCheckWifiDelay)) {
    return;
  }

  // Test Wi-Fi status
  if (WiFi.status() == WL_CONNECTED) {
    lastCheckWifiTime = millis();
    Serial.println("WiFi is connected!!!!!!!");
    tm.display("o-o-");
    delay(1000);
    have_wifi = true;
    // return;
  }

  Serial.println((String) "Checking WiFi .. " + ssid + " / " + password);
  // while (WiFi.status() != WL_CONNECTED) {
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println((String) "WiFi Status : " + (String)WiFi.status());
    if ((millis() - lastCheckWifiTime) < wifi_time_limit) {
      Serial.print(">");
      delay(500);
      have_wifi = false;
    } else {
      break;
    }
  }
  Serial.println("Checking WiFi done ..");
  if (WiFi.status() == WL_CONNECTED) {
    have_wifi = true;
    Serial.println("WiFi Connected!!!!!!!!");
  } else {
    Serial.println((String) "... Can not connect to " + ssid + ". Restart ESP32.");
    // ESP.restart();
  }
  lastCheckWifiTime = millis();
}


void send_line() {
  // 異常3次以下返回，不發Line
  abnormalCount++;
  if (abnormalCount <= 3) return;
  // Line已發送過，不再發送
  if (Alerted == true) return;
  
  if ((last_send_line_time > 0) and ((millis() - last_send_line_time) < send_cloud_time_limit)) {

    Serial.println("line Time limit....");
    Serial.print(last_send_line_time);
    return;
  }
  Alerted = true;  //Line已發送
  char host[] = "notify-api.line.me";

  String token = "2wJPJZhq5Axd0PG9HnF5Xx31MG0dV7YauHdZh4Hulwz";
  String message = "目前魚缸";
  message += "\n溫度 C=" + temperatureC + " *C";
  message += "\n溫度 F=" + temperatureF + " *F";
  if (relay_output) {
    message += "\nRelay =開";
  } else {
    message += "\nRelay =關";
  }
  message += "\n開啟 Relay 溫度=" + String(high_temp);
  message += "\n關閉 Relay 溫度=" + String(low_temp);
  Serial.println(message);

  if (client.connect(host, 443)) {
    int LEN = message.length();
    //1.傳遞網站
    String url = "/api/notify";  //Line API網址
    client.println("POST " + url + " HTTP/1.1");
    client.print("Host: ");
    client.println(host);  //Line API網站
    //2.資料表頭
    client.print("Authorization: Bearer ");
    client.println(token);
    //3.內容格式
    client.println("Content-Type: application/x-www-form-urlencoded");
    //4.資料內容
    client.print("Content-Length: ");
    client.println(String((LEN + 8)));  //訊息長度
    client.println();
    client.print("message=");
    client.println(message);  //訊息內容
    //等候回應
    delay(2000);
    String response = client.readString();
    //顯示傳遞結果
    Serial.println(response);
    client.stop();  //斷線，否則只能傳5次
    last_send_line_time = millis();
  } else {
    //傳送失敗
    Serial.println("connected fail");
  }
}


void send_ifttt() {
  
  String url = "http://maker.ifttt.com/trigger/storage508/with/key/bMrpjWsQxop14jbc5_rxtl";
   if ((last_send_ifttt_time > 0) and ((millis() - last_send_ifttt_time) < send_cloud_time_limit)) {
    Serial.println("IFTTT Time limit....");
    return;
  }

  Serial.println((String) "Start web page connect .... ");
  HTTPClient http;

  // 以HTTP Get參數方式補入網址後方
  String send_url = url + "?value1=" + temperatureC + "&value2=" + temperatureF + "&value3=" + String(relay_output);
  //http client取得網頁內容
  http.begin(send_url);
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    //讀取網頁內容到payload
    String payload = http.getString();
    //將內容顯示出來
    Serial.print((String) "Web content =");
    Serial.println((String) payload);
    
    last_send_ifttt_time = millis();
  } else {
    //讀取失敗
    Serial.println((String) "Web send fail .....");
  }
  http.end();
}

void send_thingspeak() {
  if ((last_send_cloud_time > 0) and ((millis() - last_send_cloud_time) < send_cloud_time_limit)) {
    Serial.println("Thingspeak Time limit....");
    return;
  }
  String url = "https://api.thingspeak.com/update?api_key=NAP4KZR5ZBHYWGJL";
  // if (have_wifi) {
  Serial.println("Start ThingSpeak connect ...");
  HTTPClient http;
  String send_url = url + "&field1=" + temperatureC + "&field2=" + temperatureF + "&field3=" + (String)relay_output + "&field4=" + (float)high_temp + "&field5=" + (float)low_temp;
  Serial.println((String) "Send data to ThingSpeak ..." + send_url);
  //http client取得網頁內容
  http.begin(send_url);
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    //讀取網頁內容到payload
    String payload = http.getString();
    //將內容顯示出來
    Serial.print((String) "ThingSpeak Web content=");
    Serial.println(payload);
  } else {
    //讀取失敗
    Serial.println((String) "ThingSpeak Web send fail ...");
  }
  http.end();
  last_send_cloud_time = millis();

}

void setup() {
  // Serial port for debugging purposes

  Serial.begin(9600);
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

  // check_wifi();

  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi .. ");
  lastTime = millis();
  int try_count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(String(WiFi.status()));
    delay(1000);
    if (try_count++ > 20) {
      ESP.restart();
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi Connected!");
    have_wifi = true;
  } else {
    Serial.print((String) "... Can not connect to " + ssid + ".");
  }

  lastTime = millis();
  // Print ESP Local IP Address
  // IPAddress local_ip = WiFi.localIP().toString();
  local_ip = WiFi.localIP().toString();

  client.setInsecure();  // 使用ESP32 1.0.6核心須加上這句避免SSL問題

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

        send_line();

      } else if (cc <= low_temp) {
        Serial.println("Below low");
        // digitalWrite(RELAY, LOW);
        relay_output = LOW;
        set_relay();
      } else {
        abnormalCount = 0;
        Alerted = false;

        Serial.println("Temperature is OK!");
      }
      lastTime = millis();
      send_thingspeak();
      send_ifttt();
      // check_wifi();
    }
  }
}