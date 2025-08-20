// Regulace vlhkosti vzduchu v teráriu pomocí systému Arduino Nano v3

// Knihovny
#include "Adafruit_Sensor.h"
#include "Arduino.h"
#include "DHT.h"
#include "LiquidCrystalFast.h"

typedef unsigned short USHORT;
typedef unsigned long int ULONG;

// Prototypy funkcí
void vypisHodnotuSenzoru();
void kontrolaHladinyVody(bool &hladina);
void kontrolaVlhkosti(USHORT &vlh);
void zapniCerpadlo(bool &hladina);
void zapipej();

// Definice digitálních vstupů
const USHORT zelenaLedPin       = 0;
const USHORT zlutaLedPin        = 1;
const USHORT cervenaLedPin      = 2;
const USHORT cerpadloPin        = 3;
const USHORT bzucakPin          = 4;
const USHORT senzorVlhkostiPin  = 5;
const USHORT senzorHladinyPin   = 6;

// Definice pinů pro LCD displej
const USHORT rs_pin = 11;
const USHORT e_pin  = 12;
const USHORT d4_pin = 10;
const USHORT d5_pin =  9;
const USHORT d6_pin =  8;
const USHORT d7_pin =  7;

// Globální promenné
USHORT teplota, vlhkost;
bool stavHladinyVody;

// PARAMETRY PRO REGULACI
// Hodnoty těchto glob. proměnných lze libovolně měnit:
const USHORT minHladinaVlhkosti     = 60;        // vlhkost v %
const USHORT casSpusteniCerpadla    = 6000;      // 6s
const ULONG casKontrolyVlhkosti     = 180000;    // 3 min
const ULONG casKontrolyDoplneniVody = 300000;    // 5 min

// Nastavení typu DHT senzoru
#define typSenzoru DHT11

// Deklarace senzoru vlhkosti
DHT senzor(senzorVlhkostiPin, typSenzoru);

// Deklarace LCD displeje
LiquidCrystalFast LCD(rs_pin, e_pin, d4_pin, d5_pin, d6_pin, d7_pin);

//----------------------------------------------------------------------------

void setup() {
  // Inicializace vstupů
  pinMode(cerpadloPin, OUTPUT);
  pinMode(bzucakPin, OUTPUT);
  pinMode(senzorHladinyPin, INPUT);

  // Komunikace po sériové lince rychlostí 9600 baud
  Serial.begin(9600);

  // Zapnutí komunikace se senzorem vlhkosti
  senzor.begin();

  // Inicializace LCD displeje (první spuštění)
  LCD.begin(16, 2);
  LCD.setCursor(1, 0);
  LCD.print("Aut. regulace");
  LCD.setCursor(4, 1);
  LCD.print("vlhkosti");
  delay(1000);
}

//----------------------------------------------------------------------------

void loop() {
  // Načtení hodnot ze senzoru vlhkosti
  teplota = senzor.readTemperature();
  vlhkost = senzor.readHumidity();

  // Kontrola DHT senzoru (kontrola komunikace s Arduinem)
  if (vlhkost == 0 || teplota == 0) {
    
    // Výstup na LCD
    LCD.begin(16, 2);
    LCD.setCursor(0, 0);
    LCD.print("Chyba DHT");
    LCD.setCursor(0, 1);
    LCD.print("senzoru!");
    delay(2000);

    LCD.begin(16, 2);
    LCD.setCursor(0, 0);
    LCD.print("Zkontrolujte");
    LCD.setCursor(0, 1);
    LCD.print("senzor.");

    // Výstup do konzole
    Serial.println("Chyba cteni z DHT senzoru!");
    delay(2000);
  } 
  else {
    vypisHodnotuSenzoru();

    stavHladinyVody = digitalRead(senzorHladinyPin);

    // Kontrola hladiny vody nádrži
    kontrolaHladinyVody(stavHladinyVody);

    // Kontrola a regulace vlhkosti vzduchu v teráriu
    kontrolaVlhkosti(vlhkost);

    if (vlhkost >= minHladinaVlhkosti && stavHladinyVody == true) {

      // Výstup na LCD
      LCD.setCursor(0, 1);
      LCD.print("                ");
      LCD.setCursor(0, 1);
      LCD.print("Vse je OK!");

      // Výstup do konzole
      Serial.println("Vse je OK!");
    }

    // Pro lepší čitelnost výstupu v konzoli
    Serial.println(" ");
    delay(2000);
  }
}

// Definice funkcí--------------------------------------------------------

void vypisHodnotuSenzoru() {
  // Výstup na LCD
  LCD.begin(16, 2);

  LCD.setCursor(0, 0);
  LCD.print("V: ");

  LCD.setCursor(3, 0);
  LCD.print(vlhkost);

  LCD.setCursor(5, 0);
  LCD.print("%, ");

  LCD.setCursor(8, 0);
  LCD.print("T: ");
  LCD.setCursor(11, 0);
  LCD.print(teplota);
  LCD.setCursor(13, 0);
  LCD.print((char)223);
  LCD.setCursor(14, 0);
  LCD.print("C");

  // Výstup do konzole
  Serial.print("Vlhkost: ");
  Serial.print(vlhkost);
  Serial.print(" %, ");
  Serial.print("teplota: ");
  Serial.print(teplota);
  Serial.println(" C");
}

void kontrolaHladinyVody(bool &hladina) {
  if (hladina == true) {
    digitalWrite(zlutaLedPin, LOW);

  } 
  else {
    digitalWrite(zlutaLedPin, HIGH);

    // Dokud obsluha nedoplní vodu:
    while (!hladina) {
      zapipej(); // 3x pipnuti z piezo repráčku

      // Výstup na LCD
      LCD.begin(16, 2);
      LCD.setCursor(0, 0);
      LCD.print("Dosla voda");
      LCD.setCursor(0, 1);
      LCD.print("Cekam 5 min...");

      // Výstup do konzole
      Serial.println("- Cekam na doplneni vody (5 min)...");

      delay(casKontrolyDoplneniVody);

      // Kontrola hladiny pro ukončení cyklu while
      stavHladinyVody = digitalRead(senzorHladinyPin);
      if (stavHladinyVody) {
        LCD.begin(16, 2);
        break;
      }
    }
  }
}

void zapniCerpadlo(bool &hladina) {
  if (hladina == true) {

    // Výstup na LCD
    LCD.setCursor(0, 1);
    LCD.print("               ");
    LCD.setCursor(0, 1);
    LCD.print("Zavlazuji...");

    // Zapnutí čerpadla
    digitalWrite(cerpadloPin, HIGH); // zap
    delay(casSpusteniCerpadla);
    digitalWrite(cerpadloPin, LOW); // vyp

    // Výstup na LCD
    LCD.begin(16, 2);
    LCD.setCursor(0, 0);
    LCD.print("Kontroluji");
    LCD.setCursor(0, 1);
    LCD.print("vlhkost (3 min)");

    // Výstup pro konzoli
    Serial.println("+ Kontroluji vlhkost (3 min)...");
    delay(casKontrolyVlhkosti);

  } 
  else {
    digitalWrite(cerpadloPin, LOW);
    // Serial.println("- Nemohu zavlazovat - dosla voda.");
  }
}

void kontrolaVlhkosti(USHORT &vlh) {
  if (vlh > minHladinaVlhkosti) {
    // Vlhkost je OK
    digitalWrite(zelenaLedPin, HIGH);
    digitalWrite(cervenaLedPin, LOW);

  } 
  else {
    // Vlhkost je nízká
    digitalWrite(zelenaLedPin, LOW);
    digitalWrite(cervenaLedPin, HIGH);

    zapniCerpadlo(stavHladinyVody);
  }
}

void zapipej() {
  for (USHORT i = 0; i < 3; i++) {
    digitalWrite(bzucakPin, HIGH);
    delay(450);
    digitalWrite(bzucakPin, LOW);
    delay(200);
  }
}
