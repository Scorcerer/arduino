#include <Wire.h>
#include <OneWire.h>
#include <LiquidCrystal.h>
#include <Time.h>
#include <DS1307RTC.h>
#include <avr/wdt.h>

//Definicje, definicje:
LiquidCrystal lcd(4,5,6,7,8,9,10,11,12,13); // PINy wyświetlacza LCD...
const int wiersze = 4, kolumny = 16; // I jego geometria

//Tablice stanu przekaźników + dwie funkcje do zarządzania przekaźnikami + adres shielda:
volatile bool relaySet[4]={true,true,true,true};
volatile bool relayChan[4]={false,false,false,false};
#define relay B0100000

void Relay_on(byte _data ) {

  byte _data2;
  Wire.requestFrom(relay, 1);
  if(Wire.available()) {
    _data2 = Wire.read();
//  }
  Wire.beginTransmission(relay);
  Wire.write(_data2 | 1<<_data);
  Wire.endTransmission();
  relaySet[_data]=true;
  lcd.setCursor(12,_data);lcd.print("1");
  }
}

void Relay_off(byte _data ) {

  byte _data2;
  Wire.requestFrom(relay, 1);
  if(Wire.available()) {
    _data2 = Wire.read();
//  }
  Wire.beginTransmission(relay);
  Wire.write(_data2 & ~(1<<_data));
  Wire.endTransmission();
  relaySet[_data]=false;
  lcd.setCursor(12,_data);lcd.print("0");
  }
}

// Funkcja do ustawiania przekaźników jak należy:
void setRelay(){
    for (byte i=0;i<=4;i++) {
      if(relaySet[i]!=relayChan[i]){
        if(relayChan[i]) Relay_on(i); else Relay_off(i);
      };
    };
    for(byte i=0; i<4 ; i++) {lcd.setCursor(12,i);lcd.print(int(relaySet[i]));}
}

//############## TEMPERATURA
OneWire ds(2); // 1-Wire na pinie 2
//OneWire gora(3); //1-wire na pinie 3

byte TempG[8] = { 0x28, 0x38, 0xD0, 0x76, 0x04, 0x00, 0x00, 0x33 }; // Adres Sensora nr1
byte TempD[8] = { 0x28, 0x5D, 0x15, 0xD8, 0x02, 0x00, 0x00, 0x86 }; // Adres Sensora nr2
byte TempCWU[8] = { 0x28, 0x33, 0x8d, 0x45, 0x04, 0x00, 0x00, 0x72 }; // Adres Sensora nr3
byte TempSol[8] = { 0x28, 0xB4, 0xED, 0x53, 0x04, 0x00, 0x00, 0x3D }; // Adres sensora nr4
volatile float TempTable[4];

void getTemp(byte addr[],volatile float TempTable[],int index){

  byte data[12];
  ds.reset();
  ds.select(addr);
  ds.write(0x44, 0);   // Wysłanie żądania odczytu temperatury
  delay(100);          // Tutaj ustawiamy czas oczekiwania na odczyt temperatury
  ds.reset();
  ds.select(addr);    
  ds.write(0xBE);      // Odczyt temperatury
  for (byte i = 0; i < 9; i++) data[i] = ds.read();
  int16_t raw = (data[1] << 8) | data[0];

  TempTable[index] = (float)raw / 16.0;
}

void updateTemp(volatile float wynikowa[]){
  getTemp(TempG,wynikowa,0);
  getTemp(TempD,wynikowa,1);
  getTemp(TempCWU,wynikowa,2);
  getTemp(TempSol,wynikowa,3);
}

void printTemp()
{
  lcd.setCursor(6,0);lcd.print(TempTable[0]);
  lcd.setCursor(6,1);lcd.print(TempTable[1]);
  lcd.setCursor(6,2);lcd.print(TempTable[2]);
  lcd.setCursor(6,3);lcd.print(TempTable[3]);
}

void prep()
{
  lcd.setCursor(0,0);lcd.print("TempG ");
  lcd.setCursor(0,1);lcd.print("TempD ");
  lcd.setCursor(0,2);lcd.print("ZbCWU ");
  lcd.setCursor(0,3);lcd.print("ZbSol ");
}

void printTime()
{
  if (timeStatus() == timeSet){
    lcd.setCursor(14,0); if (hour()<10) lcd.print("0"); lcd.print(hour());
    lcd.setCursor(14,1); if (minute()<10) lcd.print("0"); lcd.print(minute());
    lcd.setCursor(14,2); if (second()<10) lcd.print("0"); lcd.print(second());
    lcd.setCursor(15,3); lcd.print(weekday());
   } else {
    lcd.setCursor(14,0);lcd.print("Na");
    lcd.setCursor(14,1);lcd.print("Na");
    lcd.setCursor(14,2);lcd.print("Na");
   }
  
}

void checkTemp( byte level, float temp) {
  if ( TempTable[level] <= temp-0.2 ) relayChan[level]=true;
  else { if (TempTable[level] >= temp+0.2) relayChan[level]=false;}
}

//############    TABLICE CZASU
const bool grzanie_pietro[2][24][4]={
  { // workdays
    {0,0,0,0}, // 00
    {0,0,0,0}, // 01
    {0,0,0,0}, // 02
    {0,0,0,0}, // 03
    {0,0,0,0}, // 04
    {0,0,0,0}, // 05
    {1,1,1,1}, // 06
    {1,1,1,1}, // 07
    {1,1,1,1}, // 08
    {0,0,0,0}, // 09
    {0,0,0,0}, // 10
    {0,0,0,0}, // 11
    {1,1,1,1}, // 12
    {1,1,1,1}, // 13
    {0,0,0,0}, // 14
    {0,0,0,0}, // 15
    {0,0,0,0}, // 16
    {0,0,0,0}, // 17
    {1,1,1,1}, // 18
    {1,1,1,1}, // 19
    {1,1,1,1}, // 20
    {1,1,1,1}, // 21
    {0,0,0,0}, // 22
    {0,0,0,0}  // 23
  },
  { // weekend
    {0,0,0,0}, // 00
    {0,0,0,0}, // 01
    {0,0,0,0}, // 02
    {0,0,0,0}, // 03
    {0,0,0,0}, // 04
    {0,0,0,0}, // 05
    {0,0,0,0}, // 06
    {1,1,1,1}, // 07
    {1,1,1,1}, // 08
    {0,0,0,0}, // 09
    {0,0,0,0}, // 10
    {0,0,0,0}, // 11
    {1,1,1,1}, // 12
    {1,1,1,1}, // 13
    {0,0,0,0}, // 14
    {0,0,0,0}, // 15
    {0,0,0,0}, // 16
    {0,0,0,0}, // 17
    {1,1,1,1}, // 18
    {1,1,1,1}, // 19
    {1,1,1,1}, // 20
    {1,1,1,1}, // 21
    {0,0,0,0}, // 22
    {0,0,0,0}  // 23
  },
};
bool grzanie_parter[24][4]={
  {0,0,0,0}, // 00
  {0,0,0,0}, // 01
  {0,0,0,0}, // 02
  {0,0,0,0}, // 03
  {0,0,0,0}, // 04
  {0,0,0,0}, // 05
  {1,1,1,1}, // 06
  {1,1,1,1}, // 07
  {0,0,0,0}, // 08
  {0,0,0,0}, // 09
  {0,0,0,0}, // 10
  {0,0,0,0}, // 11
  {0,0,0,0}, // 12
  {0,0,0,0}, // 13
  {0,0,0,0}, // 14
  {0,0,0,0}, // 15
  {0,0,0,0}, // 16
  {0,0,0,0}, // 17
  {0,0,0,0}, // 18
  {0,0,0,0}, // 19
  {0,0,0,0}, // 20
  {1,1,1,1}, // 21
  {1,1,1,1}, // 22
  {0,0,0,0}  // 23
};
//############    TIMERY

volatile int tcwu = 0;
volatile int t2 = 0;

ISR(TIMER1_COMPA_vect){
  if (relayChan[3]==true && tcwu <= 225) tcwu++;  // Jeżeli mieszamy CWU, czekamy do końca
  else {                                          // Jeśli to koniec, to:
    if (relayChan[3]==true) {                     // Niestety trzeba upewnić się że mieszamy :(
      if (TempTable[2]+5 <= TempTable[3]) {       // Jeżeli kończymy mieszać, sprawdzamy czy nie lepiej jest mieszać dalej
        tcwu = 0;                                 // Widocznie warto mieszać dalej, mieszamy więc przez następne 15min
        return;                                   // Można też ustawić tcwu na więcej niż 0 i dodatkowo mieszać krócej :>
      } else {
        relayChan[3]=false;                       // Jeśli kończymy mieszać i nie warto dalej, wyłączamy przekaźnik
        tcwu = 0;                                 // I zerujemy licznik, żeby potem mieszać 15min
        return;
      }
    }
  }
  
}


ISR(TIMER2_COMPA_vect){
  if( t2 >= 200 ) {
  //  if (TempTable[2]+5 <= TempTable[3]) relayChan[3]=true;  // Tutaj odpalamy mieszanie CWU
    if (relayChan[0] || relayChan[1]) relayChan[2]=true;
    else relayChan[2]=false;
    t2 = 0;
  } else t2++;
    
}

//############# PROGRAM

void setup(void)
{
  wdt_enable(WDTO_8S);
  wdt_reset();
  setSyncProvider(RTC.get);
  setRelay(); //zerowanie przekaźników po resecie.
  lcd.begin(kolumny, wiersze); // teraz LCD
  lcd.clear();
  lcd.setCursor(5,0); lcd.print("Witaj!");
  delay(1000);
  prep();

 //ustawienie Timera1 (16bit):
 cli();
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1  = 0;
    OCR1A  = 62500;
    TCCR1B |= (1 << WGM12); //CTC
    TCCR1B |= (1 << CS12) | (1 << CS10);
    TIMSK1 |= (1 << OCIE1A);
  sei();

  //ustawienie Timera2 (8bit):
  cli();
    TCCR2A = 0;
    TCCR2B = 0;
    TCNT2  = 0;
    OCR2A  = 157;
    TCCR2A |= (1 << WGM21); //CTC
    TCCR2B |= (1 << CS12) | (1 << CS10);
    TIMSK2 |= (1 << OCIE1A);
  sei();

}

void loop(void)
{
  wdt_reset();
  updateTemp(TempTable);
  printTime();
  printTemp();

    // Ustawiamy grzanie na pietrze:
  if ( 1 < weekday() < 7){
    if(grzanie_pietro[0][hour()][minute()/15]){ checkTemp(0,20);} else checkTemp(0,18);
  } else {
    if(grzanie_pietro[1][hour()][minute()/15]){ checkTemp(0,20);} else checkTemp(0,18);
  }
    //Ustawiamy grzanie na parterze:
  if(grzanie_parter[hour()][minute()/15]){ checkTemp(1,18);} else checkTemp(1,16);
    
  setRelay();
  delay(200);

}
