//#define ThermistorPin = A0; // for Arduino microcontroller
//#define ThermistorPin = A0; // for ESP8266 microcontroller
//#define ThermistorPin = 4;  // for ESP32 microcontroller

int ThermistorPin=4;
int Vo;
float R1 = 10000; // value of R1 on board
float logR2, R2, T;

//steinhart-hart coeficients for thermistor
float c1 = 0.001129148, c2 = 0.000234125, c3 = 0.0000000876741; 

void setup() {
  Serial.begin(115200);
}

void loop() {
  Vo = analogRead(ThermistorPin);
  R2 = R1 * (1023.0 / (float)Vo - 1.0); //calculate resistance on thermistor
  logR2 = log(R2);
  T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2)); // temperature in Kelvin
  Serial.println(T); 
  T = T - 273.15; //convert Kelvin to Celcius
 // T = (T * 9.0)/ 5.0 + 32.0; //convert Celcius to Fahrenheit

  Serial.print("Temperature: "); 
  Serial.print(Vo);
  Serial.print(" --- ");
  Serial.print(T);
  Serial.println(" C"); 

  delay(500);
}
