#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30105.h"
#include "heartRate.h"


MAX30105 particleSensor;
float progressBar;
int turn,final_turn;
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
  byte rates[RATE_SIZE]; //Array of heart rates
  byte rateSpot = 0;
  long lastBeat = 0; //Time at which the last beat occurred

float beatsPerMinute=0;
int beatAvg=0,beatFinalAvg=0;

int storeBeat[40];
void setup() {
  Serial.begin(9600);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();

  Serial.println("Initializing...");

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
  void BPM();
}

void loop() {
  long irValue = particleSensor.getIR();
  if(irValue>7000) {
  for(final_turn=0;final_turn<=40;final_turn++) {
  for(turn=0;turn<50;turn++) {
    
long irValue = particleSensor.getIR();
//delay(500);
    
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
  progressBar = (2.5)*((float)final_turn);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(10, 0);
  display.setTextColor(SSD1306_WHITE);
  display.print("BPM: ");
  display.println(beatAvg);
  display.println(" ");
  display.print(progressBar);
  display.println("%");
  display.println("Completed");
  display.display();
  storeBeat[final_turn]=beatAvg;
  }
  for(final_turn=15;final_turn<=40;final_turn++) {
    beatFinalAvg += storeBeat[final_turn];
  }
  beatFinalAvg = beatFinalAvg/25;
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_WHITE);
  display.print("Final BPM :");
  display.print(beatFinalAvg);
  display.display();
  delay(5000);
  }
  else {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 10);
  display.setTextColor(SSD1306_WHITE);
  display.println("Place");
  display.println("the");
  display.println("Finger");
  display.display();
  delay(90);
  }
 
}
