#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

MAX30105 particleSensor;

#define MAX_BRIGHTNESS 255

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
uint16_t irBuffer[100]; //infrared LED sensor data
uint16_t redBuffer[100];  //red LED sensor data
#else
uint32_t irBuffer[100]; //infrared LED sensor data
uint32_t redBuffer[100];  //red LED sensor data
#endif

int normal_spo2,abnormal_spo2,critical_spo2;
int32_t bufferLength; //data length
int32_t spo2; //SPO2 value
int8_t validSPO2; //indicator to show if the SPO2 calculation is valid
int32_t heartRate; //heart rate value
int8_t validHeartRate; //indicator to show if the heart rate calculation is valid

byte pulseLED = 11; //Must be on PWM pin

int sum_normal_spo2,sum_abnormal_spo2,sum_critical_spo2;

int nofingerturn,final_sum,final_turn_spo2;
int final_arr[20];
void setup()
{
  Serial.begin(115200); // initialize serial communication at 115200 bits per second:

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();
  
  pinMode(pulseLED, OUTPUT);
  //pinMode(readLED, OUTPUT);

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println(F("MAX30105 was not found. Please check wiring/power."));
    while (1);
  }

  Serial.println(F("Attach sensor to finger with rubber band. Press any key to start conversion"));
  while (Serial.available() == 0) ; //wait until user presses a key
  Serial.read();

  byte ledBrightness = 60; //Options: 0=Off to 255=50mA
  byte sampleAverage = 4; //Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  byte sampleRate = 100; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; //Options: 69, 118, 215, 411
  int adcRange = 4096; //Options: 2048, 4096, 8192, 16384

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings
}

void loop()
{
  bufferLength = 100; //buffer length of 100 stores 4 seconds of samples running at 25sps
  
  //read the first 100 samples, and determine the signal range
  for (byte i = 0 ; i < bufferLength ; i++)
  {
    while (particleSensor.available() == false) //do we have new data?
      particleSensor.check(); //Check the sensor for new data

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample(); //We're finished with this sample so move to next sample

    Serial.print(F("red="));
    Serial.print(redBuffer[i], DEC);
    Serial.print(F(", ir="));
    Serial.println(irBuffer[i], DEC);
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_WHITE);
  display.println("Initializing");
  display.println("Please");
  display.println("Wait");
  display.display();
  }

  //calculate heart rate and SpO2 after first 100 samples (first 4 seconds of samples)
  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  //Continuously taking samples from MAX30102.  Heart rate and SpO2 are calculated every 1 second
  for(final_turn_spo2=0;final_turn_spo2<=20;final_turn_spo2++) 
  {
    //dumping the first 25 sets of samples in the memory and shift the last 75 sets of samples to the top
    for (byte i = 25; i < 100; i++)
    {
      redBuffer[i - 25] = redBuffer[i];
      irBuffer[i - 25] = irBuffer[i];
    }

    //take 25 sets of samples before calculating the heart rate.
    for (byte i = 75; i < 100; i++)
    {
      while (particleSensor.available() == false) //do we have new data?
      particleSensor.check(); //Check the sensor for new data
      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();
      particleSensor.nextSample(); //We're finished with this sample so move to next sample
    }

    //After gathering 25 new samples recalculate HR and SP02
    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
  int random_now = validSPO2;
  if(random_now == 0) {
    delay(250);
    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
    if(validSPO2 == 0) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.setTextColor(SSD1306_WHITE);
      display.println("Finger");
      display.println("Not");
      display.println("Detected");
      display.display();
    }
    continue;
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_WHITE);
  display.println("SPO2 Level:");
  display.print(spo2, DEC);
  display.display();
  Serial.println(validSPO2, DEC);
  final_arr[final_turn_spo2] = spo2;
  }
  for(final_turn_spo2=0;final_turn_spo2<=20;final_turn_spo2++) {
    if(final_arr[final_turn_spo2] > 90) {
      sum_normal_spo2 += final_arr[final_turn_spo2];
      normal_spo2++;
    }
    if(final_arr[final_turn_spo2] < 90 && final_arr[final_turn_spo2] > 60) {
      sum_abnormal_spo2 += final_arr[final_turn_spo2];
      abnormal_spo2++;
    }
    if(final_arr[final_turn_spo2] < 60) {
      sum_critical_spo2 += final_arr[final_turn_spo2];
      critical_spo2++;
    }
  }
  if(normal_spo2 > abnormal_spo2) {
    if(normal_spo2 > critical_spo2) {
      //Serial.println(sum_normal_spo2/normal_spo2);
      //Serial.println("Normal");
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.setTextColor(SSD1306_WHITE);
      display.println("Final SPO2 Level:");
      display.setTextSize(2);
      display.println(sum_normal_spo2/normal_spo2);
      display.setTextSize(1);
      display.println("Normal Level");
      display.display();
    }
    else {
      //Serial.println(sum_critical_spo2/critical_spo2);
      //Serial.println("Critical");
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.setTextColor(SSD1306_WHITE);
      display.println("Final SPO2 Level:");
      display.setTextSize(2);
      display.println(sum_critical_spo2/critical_spo2);
      display.setTextSize(1);
      display.println("Critical Level");
      display.display();
    }
  } 
  else {
   if(abnormal_spo2 > critical_spo2){
    //Serial.println(sum_abnormal_spo2/abnormal_spo2);//print abnormal 
    //Serial.println("abnormal");
    display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.setTextColor(SSD1306_WHITE);
      display.println("Final SPO2 Level:");
      display.setTextSize(2);
      display.println(sum_abnormal_spo2/abnormal_spo2);
      display.setTextSize(1);
      display.println("Mild Level");
      display.display();
   }
   else {
    //Serial.println(sum_critical_spo2/critical_spo2);// print critical
    //Serial.println("critical");
    display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.setTextColor(SSD1306_WHITE);
      display.println("Final SPO2 Level:");
      display.setTextSize(2);
      display.println(sum_critical_spo2/critical_spo2);
      display.setTextSize(1);
      display.println("Critical Level");
      display.display();
   }
  }
  delay(5000);

}
