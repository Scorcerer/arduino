#include <Wire.h>
#include <OneWire.h>
#include <LiquidCrystal_I2C.h>
#include <Time.h>
#include <DS1307RTC.h>
#include <avr/wdt.h>
#include "avr/pgmspace.h"
#include <EEPROM.h>
#include <TimeLib.h>
/* Sending time syntax:
 * Use "date +T%s\n > /dev/ttyACM0" (UTC time zone)
*/

// Definicje, definicje:
#define TIME_HEADER  "T"   // Header tag for serial time sync message
LiquidCrystal_I2C lcd(0x27,16,4);

// Zmienne dla temperatur - zeby nie szukac tego w kodzie
double pietro_work_high   = 18 ;
double pietro_work_low    = 17 ;
double pietro_weekend_high= 18 ;
double pietro_weekend_low = 17 ;

double parter_work_high   = 19   ;
double parter_work_low    = 17.5 ;
double parter_weekend_high= 19   ;
double parter_weekend_low = 17.5 ;

//Inicjalizacja wskaznika EEPROM
int eeprom_addr = 0;

//Definicja startowych adresow dla tabel w EEPROM
int pietro_work = 0;
int pietro_week = 100;
int parter_work = 200;
int parter_week = 300;

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
    for (byte i=0;i<4;i++) {
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
byte TempSol[8] = { 0x28, 0xF7, 0x7F, 0x5C, 0x06, 0x00, 0x00, 0xF0 }; // Adres sensora nr4
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

void processMessage() {
  unsigned long pctime;
  const unsigned long DEFAULT_TIME = 1543622400; // Dec 1 2018

  if(Serial.find(TIME_HEADER)) {
     pctime = Serial.parseInt();
     if( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
       setTime(pctime); // Sync Arduino clock to the time received on the serial port
     }
  }
}

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
  Serial.begin(9600);
  while (!Serial) ; // Needed for Leonardo only
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
  if (Serial.available()) {
    processMessage();
  }

    // Ustawiamy grzanie na pietrze:
  if ( 1 < weekday() && weekday() < 7){
    if(EEPROM.read(pietro_work+(hour()*4)+(minute()/15))){ checkTemp(0,pietro_work_high);} else checkTemp(0,pietro_work_low);
  } else {
    if(EEPROM.read(pietro_week+(hour()*4)+(minute()/15))){ checkTemp(0,pietro_weekend_high);} else checkTemp(0,pietro_weekend_low);
  }
    //Ustawiamy grzanie na parterze:
  if ( 1 < weekday() && weekday() < 7){
    if(EEPROM.read(parter_work+(hour()*4)+(minute()/15))){ checkTemp(1,parter_work_high);} else checkTemp(1,parter_work_low);
  } else {
    if(EEPROM.read(parter_week+(hour()*4)+(minute()/15))){ checkTemp(1,parter_weekend_high);} else checkTemp(1,parter_weekend_low);
  }
  
    //Ustawiamy mieszanie między zbiornikami CWU:
  if( 7 < hour() < 18 )
  {
    if(!relayChan[3] && (TempTable[2]+8 <= TempTable[3])) tcwu = 1;
  }
  
    
  setRelay();
  delay(200);

}
