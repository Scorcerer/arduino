#include <Wire.h>
#include <OneWire.h>
#include <LiquidCrystal_I2C.h>
#include <Time.h>
#include <DS1307RTC.h>
#include <avr/wdt.h>
#include "avr/pgmspace.h"

//Definicje, definicje:
LiquidCrystal_I2C lcd(0x27,16,4);

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
  }
}

// Funkcja do ustawiania przekaźników jak należy:
void setRelay(){
    for (byte i=0;i<=4;i++) {
      if(relaySet[i]!=relayChan[i]){
        if(relayChan[i]) Relay_on(i); else Relay_off(i);
      };
    };
//    for(byte i=0; i<4 ; i++) {lcd.setCursor(12,i);lcd.print(int(relaySet[i]));}
}

//############## TEMPERATURA
OneWire ds2(2); // 1-Wire na pinie 2
OneWire ds3(3); // Oddzielne 1-Wire na pinie 3

byte TempG[8] = { 0x28, 0x38, 0xD0, 0x76, 0x04, 0x00, 0x00, 0x33 }; // Adres Sensora nr1
byte TempD[8] = { 0x28, 0x5D, 0x15, 0xD8, 0x02, 0x00, 0x00, 0x86 }; // Adres Sensora nr2
byte TempCWU[8] = { 0x28, 0x33, 0x8d, 0x45, 0x04, 0x00, 0x00, 0x72 }; // Adres Sensora nr3
byte TempSol[8] = { 0x28, 0xB4, 0xED, 0x53, 0x04, 0x00, 0x00, 0x3D }; // Adres sensora nr4
volatile float TempTable[4];

void getTemp(OneWire ds,byte addr[],volatile float TempTable[],int index){

  byte data[12];
  ds.reset();
  ds.select(addr);
  ds.write(0x44, 0);   // Wysłanie żądania odczytu temperatury
  delay(150);          // Tutaj ustawiamy czas oczekiwania na odczyt temperatury
  ds.reset();
  ds.select(addr);    
  ds.write(0xBE);      // Odczyt temperatury
  for (byte i = 0; i < 9; i++) data[i] = ds.read();
  int16_t raw = (data[1] << 8) | data[0];

  TempTable[index] = (float)raw / 16.0;
}

void updateTemp(volatile float wynikowa[]){
  getTemp(ds2,TempG,wynikowa,0);
  getTemp(ds2,TempD,wynikowa,1);
  getTemp(ds2,TempCWU,wynikowa,2);
  getTemp(ds3,TempSol,wynikowa,3);
}

void LCDUpdate()
{
  lcd.setCursor(0,0);lcd.print("TempG "); lcd.print(TempTable[0]); lcd.print(" "); lcd.print(relaySet[0]); lcd.print(" "); if (hour()<10) lcd.print("0"); lcd.print(hour());
  lcd.setCursor(0,1);lcd.print("TempD "); lcd.print(TempTable[1]); lcd.print(" "); lcd.print(relaySet[1]); lcd.print(" "); if (minute()<10) lcd.print("0"); lcd.print(minute());
  lcd.setCursor(-4,2);lcd.print("ZbCWU "); lcd.print(TempTable[2]); lcd.print(" "); lcd.print(relaySet[2]); lcd.print(" "); if (second()<10) lcd.print("0"); lcd.print(second());
  lcd.setCursor(-4,3);lcd.print("ZbSol "); lcd.print(TempTable[3]); lcd.print(" "); lcd.print(relaySet[3]); lcd.print("  "); lcd.print(weekday());
}

void checkTemp( byte level, float temp) {
  if ( TempTable[level] <= temp-0.2 ) relayChan[level]=true;
  else { if (TempTable[level] >= temp+0.2) relayChan[level]=false;}
}

//############    TABLICE CZASU
//prog_uchar grzanie_pietro[2][24][4]={
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

volatile int tcwu = 0;                // Miejsce na czas mieszania
volatile int tsleep = 0;              // Miejsce na czas między-mieszaniami
volatile int t2 = 0;

ISR(TIMER1_COMPA_vect){
  if ( tsleep !=0 && tsleep < 450 )         // Jeśli uruchomił się timer między-mieszaniowy, musimy go przeczekać
  {
    tsleep++;                               // więc dodajemy czekanie
    tcwu = 0;                               // i usuwamy ew. zapędy do mieszania
  }
  else {
    tsleep = 0;                            // Wygląda na to że zakończyliśmy oczekiwanie, więc zerujemy jego timer.
    if ( tcwu !=0 && tcwu <= 215) tcwu++;  // Jeżeli mieszamy CWU, czekamy do końca
    else {                                          // Jeśli to koniec, to:
      if (relayChan[3]==true) {                     // Niestety trzeba upewnić się że mieszamy :(
        if (TempTable[2]+8 <= TempTable[3]) {       // Jeżeli kończymy mieszać, sprawdzamy czy nie lepiej jest mieszać dalej
          tcwu = 110;                               // Widocznie warto mieszać dalej, mieszamy więc przez następne ~7min
          return;                                   // Można też ustawić tcwu na więcej niż 0 i dodatkowo mieszać krócej :>
        } else {
          relayChan[3]=false;                       // Jeśli kończymy mieszać i nie warto dalej, wyłączamy przekaźnik
          tcwu = 0;                                 // I zerujemy licznik, żeby potem mieszać 1min
          tsleep = 1;
          return;
        }
      } else relayChan[3]=true;
    }
  }
}


ISR(TIMER2_COMPA_vect){
  if( t2 >= 200 ) {
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
  lcd.init();
  lcd.backlight();
//  lcd.clear();
  lcd.setCursor(5,0); lcd.print("Witaj!");
  delay(1000);
  LCDUpdate();
//  prep();

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
  wdt_reset();
  LCDUpdate();

    // Ustawiamy grzanie na pietrze:
  if ( 1 < weekday() < 7){
    if(grzanie_pietro[0][hour()][minute()/15]){ checkTemp(0,19);} else checkTemp(0,17);
  } else {
    if(grzanie_pietro[1][hour()][minute()/15]){ checkTemp(0,19);} else checkTemp(0,17);
  }
    //Ustawiamy grzanie na parterze:
  if(grzanie_parter[hour()][minute()/15]){ checkTemp(1,17);} else checkTemp(1,15);
  
    //Ustawiamy mieszanie między zbiornikami CWU:
  if( 7 < hour() < 18 )
  {
    if(!relayChan[3] && (TempTable[2]+8 <= TempTable[3])) tcwu = 1;
  }
  
    
  setRelay();
  delay(200);

}
