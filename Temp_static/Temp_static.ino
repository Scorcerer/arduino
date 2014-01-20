#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal.h>
#include <Time.h>
#include <DS1307RTC.h>

//Definicje, definicje:
OneWire oneWire(2); // 1-Wire na pinie 2
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature.
LiquidCrystal lcd(4,5,6,7,8,9,10,11,12,13); // PINy wyświetlacza LCD...
const int wiersze = 4, kolumny = 16; // I jego geometria
tmElements_t tm; // Coś na czasie :)
bool CWUon = false;
//volatile float Temp[4];

//Tablice stanu przekaźników + dwie funkcje do zarządzania przekaźnikami + adres shielda:
volatile bool relaySet[4]={true,true,true,true};
volatile bool relayChan[4]={false,false,false,false};
volatile int t1 = 0;
volatile int t2 = 0;

#define relay B0100000
void Relay_on(byte _data ) {

  byte _data2;
  Wire.requestFrom(relay, 1);
  if(Wire.available()) {
    _data2 = Wire.read();
  }

  Wire.beginTransmission(relay);
  Wire.write(_data2 | 1<<_data);
  Wire.endTransmission();
  relaySet[_data]=true;
  lcd.setCursor(12,_data);lcd.print("1");
}

void Relay_off(byte _data ) {

  byte _data2;
  Wire.requestFrom(relay, 1);
  if(Wire.available()) {
    _data2 = Wire.read();
  }

  Wire.beginTransmission(relay);
  Wire.write(_data2 & ~(1<<_data));
  Wire.endTransmission();
  relaySet[_data]=false;
  lcd.setCursor(12,_data);lcd.print("0");
  
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


DeviceAddress TempG = { 0x28, 0x45, 0xAB, 0x94, 0x04, 0x00, 0x00, 0xB2 }; // Adres Sensora nr1
DeviceAddress TempD = { 0x28, 0x5D, 0x15, 0xD8, 0x02, 0x00, 0x00, 0x86 }; // Adres Sensora nr2
DeviceAddress TempCWU = { 0x28, 0x33, 0x8d, 0x45, 0x04, 0x00, 0x00, 0x72 }; // Adres Sensora nr3
//DeviceAddress TempSol = { 0x28, 0xDB, 0x79, 0x45, 0x04, 0x00, 0x00, 0x43 }; // Adres Sensora nr4 //prawidłowy, juz wstawiony
DeviceAddress TempSol = { 0x28, 0xB4, 0xED, 0x53, 0x04, 0x00, 0x00, 0x3D }; // Adres tymczasowego sensora nr4
int przekaz = 0;
bool przekazON = true;

void printTemp()
{
  lcd.setCursor(6,0);lcd.print(sensors.getTempC(TempG));
  lcd.setCursor(6,1);lcd.print(sensors.getTempC(TempD));
  lcd.setCursor(6,2);lcd.print(sensors.getTempC(TempCWU));
  lcd.setCursor(6,3);lcd.print(sensors.getTempC(TempSol));
}

void prep()
{
  lcd.setCursor(0,0);lcd.print("TempG ");
  lcd.setCursor(0,1);lcd.print("TempD ");
  lcd.setCursor(0,2);lcd.print("ZbCWU ");
  lcd.setCursor(0,3);lcd.print("ZbSol ");
}

void printTime(tmElements_t zegar)
{
  if (RTC.read(zegar)){
    lcd.setCursor(14,0); if (zegar.Hour<10) lcd.print("0"); lcd.print(zegar.Hour);
    lcd.setCursor(14,1); if (zegar.Minute<10) lcd.print("0"); lcd.print(zegar.Minute);
    lcd.setCursor(14,2); if (zegar.Second<10) lcd.print("0"); lcd.print(zegar.Second);
   } else {
    lcd.setCursor(14,0);lcd.print("Na");
    lcd.setCursor(14,1);lcd.print("Na");
    lcd.setCursor(14,2);lcd.print("Na");
   }
  
}

ISR(TIMER1_COMPA_vect){
  if (relayChan[3]==true && t1 <= 225) t1++;
  else {
    if(relayChan[3]==true) {
      if (sensors.getTempC(TempCWU)+5 <= sensors.getTempC(TempSol)) {
        t1 = 0;
        return;
      }
      else {
        relayChan[3]=false;
        t1 = 0;
        return;
      }
    }
  if ((sensors.getTempC(TempCWU)+5 <= sensors.getTempC(TempSol)) && (sensors.getTempC(TempSol)) != -127.00 ) relayChan[3]=true;
  }
}

ISR(TIMER2_COMPA_vect){
  if( t2 >= 200 ){
    t2 = 0;
    
    prep();
    printTemp();
    printTime(tm);
    setRelay();
  } else t2++;
    
}

void setup(void)
{
  // Najpierw ustawiamy czujniki...
  sensors.begin();
  sensors.setResolution(TempG, 12);
  sensors.setResolution(TempD, 12);
  sensors.setResolution(TempCWU, 12);
  sensors.setResolution(TempSol, 12);
//  sensors.setWaitForConversion(false);
  setRelay(); //zerowanie przekaźników po resecie.
  lcd.clear();
  lcd.begin(kolumny, wiersze); // teraz LCD
  lcd.setCursor(5,0); lcd.print("Witaj!");
  delay(1000);


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
      sensors.requestTemperatures();
/*      Temp[0]=sensors.getTempC(TempG);
      Temp[1]=sensors.getTempC(TempD);
      Temp[2]=sensors.getTempC(TempCWU);
      Temp[3]=sensors.getTempC(TempSol);
  */
  delay(100);
} 
