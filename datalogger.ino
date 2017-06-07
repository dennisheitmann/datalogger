#include <Wire.h>
#include <RTClib.h>
#include <PinChangeInterruptSettings.h>
#include <PinChangeInterrupt.h>
#include <PinChangeInterruptPins.h>
#include <PinChangeInterruptBoards.h>

#include <SPI.h>
#include <SD.h>

#include <Sodaq_SHT2x.h>
#include <Adafruit_BMP085.h>

Adafruit_BMP085 bmp;

RTC_DS3231 rtc;

const int chipSelect = 4;

// Connect the Pin_3 of DSM501A to Arduino 5V
// Connect the Pin_5 of DSM501A to Arduino GND
// Connect the Pin_2 of DSM501A to Arduino D12

#define PM25PIN 2 //DSM501A input D2

unsigned long starttime = 0;
unsigned long measurementtime = 0;
unsigned long lowpulseoccupancy = 0;
unsigned long duration = 0;
unsigned long sampletime_ms = 30000;

float ratio = 0;
float concentration = 0;
float weight = 0;
float temp = 20;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  Wire.begin();

  bmp.begin();

  if (! rtc.begin()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  DateTime now = rtc.now();
  Serial.print(now.year(), DEC);
  Serial.print("-");
  Serial.print(now.month(), DEC);
  Serial.print("-");
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(":");
  Serial.print(now.minute(), DEC);
  Serial.print(":");
  Serial.print(now.second(), DEC);
  Serial.println(" Uhr");

  Serial.print("Initializing SD card...");
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");

  pinMode(PM25PIN, INPUT);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(PM25PIN), pm25pinchange, CHANGE);
  measurementtime = micros();
}

void pm25pinchange()
{
  if (starttime == 0)
  {
    starttime = micros();
  }
  uint8_t trigger = getPinChangeInterruptTrigger(digitalPinToPCINT(PM25PIN));
  if (trigger == RISING)
  {
    lowpulseoccupancy += (micros() - starttime);
  }
  else if (trigger == FALLING)
  {
    starttime = micros();
  }
}

void daten_schreiben() {
  DateTime now = rtc.now();
  String dataString = "";
  dataString += String(now.unixtime());
  dataString += String(";");
  dataString += String(now.year());
  dataString += String("-");
  dataString += String(now.month());
  dataString += String("-");
  dataString += String(now.day());
  dataString += String(" ");
  dataString += String(now.hour());
  dataString += String(":");
  dataString += String(now.minute());
  dataString += String(":");
  dataString += String(now.second());
  dataString += String(";");
  dataString += String(weight);
  dataString += String(";");
  dataString += String(SHT2x.GetTemperature());
  dataString += String(";");
  dataString += String(SHT2x.GetHumidity());
  dataString += String(";");
  dataString += String(bmp.readTemperature());
  dataString += String(";");
  dataString += String(bmp.readSealevelPressure(535) * 0.01);
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    // print to the serial port too:
    Serial.print("SD: ");
    Serial.println(dataString);
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  duration = micros() - measurementtime;
  if (duration > (sampletime_ms * 1000))
  {
    disablePinChangeInterrupt(digitalPinToPinChangeInterrupt(PM25PIN));
    ratio = (float(lowpulseoccupancy) / float(duration)) * 100.0;
    // using adapted: http://bayen.eecs.berkeley.edu/sites/default/files/conferences/Low-cost_coarse_airborne.pdf
    //    concentration = 848.0 * ratio + 0.457;
    // using adapted: https://github.com/richardhmm/DIYRepo/blob/master/arduino/libraries/DSM501/DSM501.cpp
    weight = 10 * (0.1776 * pow((ratio / 100), 3) - 0.24 * pow((ratio / 100), 2) + 94.003 * (ratio / 100));
    //    Serial.print("lowpulseoccupancy:");
    //    Serial.print(lowpulseoccupancy);
    //    Serial.print(" ratio:");
    //    Serial.print(ratio);
    //    Serial.print(" concentration:");
    //    Serial.print(concentration);
    //    Serial.print(" weight:");
    //    Serial.print(weight);
    //    Serial.println();
    daten_schreiben();
    lowpulseoccupancy = 0;
    starttime = 0;
    enablePinChangeInterrupt(digitalPinToPinChangeInterrupt(PM25PIN));
    measurementtime = micros();
  }
}
