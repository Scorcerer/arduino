
#include <LiquidCrystal.h>

LiquidCrystal lcd(3,4,5,6,7,8,9,10,11,12,13);
const int wiersze = 4, kolumny = 16;
int licznik = 0;

void setup()
{
  // init
  lcd.begin(kolumny, wiersze);
  
  lcd.setCursor(5,0);
  lcd.print("Witaj!");
  lcd.setCursor(3,1);
  lcd.print("DEC = BIN");
  lcd.setCursor(3,2);
  lcd.print("OCT = HEX");
  delay(5000);
}

void loop()
{
  lcd.clear();
  lcd.print(licznik,DEC);
  lcd.print("(10)");
  lcd.setCursor(15,0);
  lcd.print("=");
  
  lcd.setCursor(0,1);
  lcd.print(licznik,BIN);
  lcd.print("(2)");
  lcd.setCursor(15,1);
  lcd.print("=");
  
  lcd.setCursor(0,2);
  lcd.print(licznik,OCT);
  lcd.print("(8)");
  lcd.setCursor(15,2);
  lcd.print("=");
  
  lcd.setCursor(0,3);
  lcd.print(licznik,HEX);
  lcd.print("(16)");
  
  ++licznik;
  
  if (licznik >= 4095)
    licznik = 0;
    
  delay(500);
}
