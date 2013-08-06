#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal.h>
#include <Time.h>
#include <DS1307RTC.h>


OneWire oneWire(2); // Setup a oneWire instance to communicate with any OneWire devices
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature.

DeviceAddress Temp1 = { 0x28, 0x45, 0xAB, 0x94, 0x04, 0x00, 0x00, 0xB2 };
DeviceAddress Temp2 = { 0x28, 0x5D, 0x15, 0xD8, 0x02, 0x00, 0x00, 0x86 };
DeviceAddress Temp3 = { 0x28, 0x33, 0x8d, 0x45, 0x04, 0x00, 0x00, 0x72 };
DeviceAddress Temp4 = { 0x28, 0xDB, 0x79, 0x45, 0x04, 0x00, 0x00, 0x43 };

LiquidCrystal lcd(4,5,6,7,8,9,10,11,12,13);
const int wiersze = 4, kolumny = 16;

void setup(void)
{
  // Start up the library
  sensors.begin();
  sensors.setResolution(Temp1, 12);
  sensors.setResolution(Temp2, 12);
  sensors.setResolution(Temp3, 12);
  sensors.setResolution(Temp4, 12);
  
  lcd.begin(kolumny, wiersze);
  
  lcd.setCursor(5,0);
  lcd.print("Witaj!");
  sensors.requestTemperatures();
  
//  Serial.begin(9600);
//  use();
//  read_RTC();
  
  delay(2000);
  lcd.clear(); prep();
}

void prep(void)
{
  lcd.setCursor(0,0);lcd.print("Temp1: ");
  lcd.setCursor(0,1);lcd.print("Temp2: ");
  lcd.setCursor(0,2);lcd.print("Temp3: ");
  lcd.setCursor(0,3);lcd.print("Temp4: ");
}

void printTemp(void)
{
  lcd.setCursor(7,0);lcd.print(sensors.getTempC(Temp1));
  lcd.setCursor(7,1);lcd.print(sensors.getTempC(Temp2));
  lcd.setCursor(7,2);lcd.print(sensors.getTempC(Temp3));
  lcd.setCursor(7,3);lcd.print(sensors.getTempC(Temp4));
}

void printTime(tmElements_t zegar)
{
  if (RTC.read(zegar)){
    lcd.setCursor(14,0);lcd.print(zegar.Hour);
    lcd.setCursor(14,1);lcd.print(zegar.Minute);
    lcd.setCursor(14,2);lcd.print(zegar.Second);
   } else {
    lcd.setCursor(14,0);lcd.print("Na");
    lcd.setCursor(14,1);lcd.print("Na");
    lcd.setCursor(14,2);lcd.print("Na");
   }
  
}

void loop(void)
{
  tmElements_t tm;
  sensors.requestTemperatures();
  printTemp();
  printTime(tm);
  
//  delay(100);
} 
