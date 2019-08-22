#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
#include <Adafruit_HTU21DF.h>
#include <TimeLib.h>
#include <DS1307RTC.h>

const int chipSelect = 10;

LiquidCrystal_PCF8574 lcd(0x27); // set the LCD address to 0x27 for a 16 chars and 2 line display
Adafruit_HTU21DF htu = Adafruit_HTU21DF();

unsigned long duration;
unsigned long measurementtime;
unsigned long sampletime = 60; // Seconds

bool uhr;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.print("Initializing SD card...");
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
  if (!htu.begin()) {
    Serial.println("Couldn't find sensor!");
    while (1);
  }
  lcd.setBacklight(255);
  lcd.home();
  lcd.clear();
  lcd.print("Temp(C); Humi(%)");
  lcd.setCursor(0, 1);
  lcd.print("Dennis Heitmann");
}

String messwert_erfassen() {
  float temp = htu.readTemperature();
  char temp_str[7];
  dtostrf(temp, 6, 2, temp_str);
  float humi = htu.readHumidity();
  char humi_str[7];
  dtostrf(humi, 6, 2, humi_str);
  String messwert;
  messwert.concat(temp_str);
  messwert.concat(";");
  messwert.concat(humi_str);
  return messwert;
}

void lcd_anzeige(String messwert) {
  String timeString = "";
  tmElements_t tm;
  if (RTC.read(tm)) {
    timeString += "d:";
    timeString += String(tm.Day);
    timeString += String(". t:");
    timeString += String(tm.Hour);
    timeString += String(":");
    timeString += String(tm.Minute);
    timeString += String(":");
    timeString += String(tm.Second);
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  if (uhr == 0) {
    lcd.print(timeString);
    uhr = 1;
  } else {
    lcd.print("Temp(C); Humi(%)");
    uhr = 0;
  }
  lcd.setCursor(0, 1);
  lcd.print(" ");
  lcd.print(messwert);
  lcd.print(" ");
}

void daten_schreiben(String messwert) {
  String dataString = "";
  tmElements_t tm;
  if (RTC.read(tm)) {
    dataString += String(tmYearToCalendar(tm.Year));
    dataString += String("-");
    dataString += String(tm.Month);
    dataString += String("-");
    dataString += String(tm.Day);
    dataString += String(" ");
    dataString += String(tm.Hour);
    dataString += String(":");
    dataString += String(tm.Minute);
    dataString += String(":");
    dataString += String(tm.Second);
  }
  dataString += String("; ");
  dataString += String(messwert);
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

void loop(void) {
  duration = millis() - measurementtime;
  if (duration > (sampletime * 1000))
  {
    String messwert = messwert_erfassen();
    lcd_anzeige(messwert);
    daten_schreiben(messwert);
    measurementtime = millis();
  }
}
