#include <LiquidCrystal.h>

LiquidCrystal lcd(3,4,5,6,7,8,9,10,11,12,13);
const int wiersze = 4, kolumny = 16;

void setup()
{
  // init
  lcd.begin(kolumny, wiersze);
  
  lcd.setCursor(5,0);
  lcd.print("Witaj!");

  delay(5000);
}

void loop()
{
  lcd.clear();
    
  delay(500);
}
