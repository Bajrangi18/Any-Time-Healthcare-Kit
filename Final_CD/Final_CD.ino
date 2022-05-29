#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <SPI.h>
#include <Wire.h>

MAX30105 particleSensor;

const char* ssid = "WifiIdRequired";
const char* password =  "WifiPassRequired";

Adafruit_MLX90614 mlx = Adafruit_MLX90614();
const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
  byte rates[RATE_SIZE]; //Array of heart rates
  byte rateSpot = 0;
  long lastBeat = 0; //Time at which the last beat occurred

float beatsPerMinute=0;
int beatAvg=0,beatFinalAvg=0;

int storeBeat[40];

String readBodyTemp() {
  return String(mlx.readObjectTempF());
}
String readBeat() {
  long irValue = particleSensor.getIR();
  if(irValue>7000) {
  for(final_turn=0;final_turn<=40;final_turn++) {
  for(turn=0;turn<50;turn++) {
    
  long irValue = particleSensor.getIR();

    
  if (checkForBeat(irValue) == true)
  { 
    Serial.println("Entered");
    //We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }
  }
  storeBeat[final_turn]=beatAvg;
  }
  for(final_turn=15;final_turn<=40;final_turn++) {
    beatFinalAvg += storeBeat[final_turn];
  }
  beatFinalAvg = beatFinalAvg/25;
 
  }
  return String.(beatFinalAvg);
}
void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password); 
  while (WiFi.status() != WL_CONNECTED) { //Check for the connection
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  Serial.println("Adafruit MLX90614 test");  
  mlx.begin();  
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");
  particleSensor.setup(); 
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0); 
  void BPM();
}


void loop() {
  if(WiFi.status()== WL_CONNECTED){
  HTTPClient http;
  http.begin("192.168.0.105/ESPServer");   // ESP32 Localhost server 
  int httpResponseCode = http.POST(readBodyTemp().c_str()); 
  int httpResponseCode = http.POST(readBeat().c_str());  
  
   if(httpResponseCode>0){
  
    String response = http.getString();                       
  
    Serial.println(httpResponseCode);   
    Serial.println(response);           
   }else{
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
   }
   http.end(); 
 }else{
    Serial.println("Error in WiFi connection");   
 }
  delay(1000);  
}
