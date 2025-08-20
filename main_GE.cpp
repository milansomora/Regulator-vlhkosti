// Bibliotheken
#include "Adafruit_Sensor.h"
#include "Arduino.h"
#include "DHT.h"
#include "LiquidCrystalFast.h"

typedef unsigned short USHORT;
typedef unsigned long int ULONG;

// Funktionsprototypen
void sensorwertAusgeben();
void wasserstandPruefen(bool &niveau);
void feuchtigkeitPruefen(USHORT &vlh);
void pumpeEinschalten(bool &niveau);
void piepen();

// Definition digitaler Eingänge
const USHORT grueneLedPin = 0;
const USHORT gelbeLedPin = 1;
const USHORT roteLedPin = 2;
const USHORT pumpePin = 3;
const USHORT summerPin = 4;
const USHORT feuchtigkeitsSensorPin = 5;
const USHORT wasserstandsSensorPin = 6;

// Definition der Pins für das LCD-Display
const USHORT rs_pin = 11;
const USHORT e_pin  = 12;
const USHORT d4_pin = 10;
const USHORT d5_pin = 9;
const USHORT d6_pin = 8;
const USHORT d7_pin = 7;

// Globale Variablen
USHORT temperatur, feuchtigkeit;
bool Wasserstand;

// Werte dieser globalen Variablen können beliebig geändert werden:
const USHORT minFeuchtigkeitsniveau    = 60;     // [%]
const USHORT pumpenlaufzeit            = 6000;   // 6s
const ULONG feuchtigkeitsPruefungszeit = 180000; // 3 min
const ULONG wasserauffuellPruefungszeit = 300000; // 5 min

// Einstellung des DHT-Sensortyps
#define sensortyp DHT11

// Deklaration des Feuchtigkeitssensors
DHT sensor(feuchtigkeitsSensorPin, sensortyp);

// Deklaration des LCD-Displays
LiquidCrystalFast LCD(rs_pin, e_pin, d4_pin, d5_pin, d6_pin, d7_pin);


void setup() {
    // Initialisierung der Eingänge
    pinMode(pumpePin, OUTPUT);
    pinMode(summerPin, OUTPUT);
    pinMode(wasserstandsSensorPin, INPUT);

    // Kommunikation über die serielle Schnittstelle mit 9600 Baud
    Serial.begin(9600);

    // Einschalten der Kommunikation mit dem Feuchtigkeitssensor
    sensor.begin();

    // Einschalten des LCD-Displays
    LCD.begin(16, 2);
    LCD.setCursor(1, 0);
    LCD.print("Aut. Regelung");
    LCD.setCursor(4, 1);
    LCD.print("der Feuchtigkeit");
    delay(1000);
}

void loop() {
    // Einlesen der Werte vom Feuchtigkeitssensor
    temperatur = sensor.readTemperature();
    feuchtigkeit = sensor.readHumidity();

    // Prüfung des DHT-Sensors (Kommunikation mit Arduino)
    if (feuchtigkeit == 0 || temperatur == 0) {
        // Ausgabe auf das LCD
        LCD.begin(16, 2);
        LCD.setCursor(0, 0);
        LCD.print("DHT Sensorfehler");
        LCD.setCursor(0, 1);
        LCD.print("!");
        delay(2000);
        LCD.begin(16, 2);
        LCD.setCursor(0, 0);
        LCD.print("Prüfen Sie");
        LCD.setCursor(0, 1);
        LCD.print("den Sensor.");

        // Ausgabe auf die Konsole
        Serial.println("Fehler beim Lesen vom DHT-Sensor!");
        delay(2000);
    }
    else {
        sensorwertAusgeben();
        Wasserstand = digitalRead(wasserstandsSensorPin);

        // Überprüfung des Wasserstands im Tank
        wasserstandPruefen(Wasserstand);

        // Prüfung und Regelung der Luftfeuchtigkeit im Terrarium
        feuchtigkeitPruefen(feuchtigkeit);

        if (feuchtigkeit >= minFeuchtigkeitsniveau && Wasserstand == true) {
            // Ausgabe auf das LCD
            LCD.setCursor(0, 1);
            LCD.print("                ");
            LCD.setCursor(0, 1);
            LCD.print("Alles ist OK!");

            // Ausgabe na die Konsole
            Serial.println("Alles ist OK!");
        }

        // Für bessere Lesbarkeit der Konsolenausgabe
        Serial.println(" ");
        delay(2000);
    }
}

void sensorwertAusgeben() {
    // Ausgabe auf das LCD
    LCD.begin(16, 2);
    LCD.setCursor(0, 0);
    LCD.print("F: ");

    LCD.setCursor(3, 0);
    LCD.print(feuchtigkeit);

    LCD.setCursor(5, 0);
    LCD.print("%, ");

    LCD.setCursor(8, 0);
    LCD.print("T: ");
    LCD.setCursor(11, 0);
    LCD.print(temperatur);
    LCD.setCursor(13, 0);
    LCD.print((char)223);
    LCD.setCursor(14, 0);
    LCD.print("C");

    // Ausgabe auf die Konsole
    Serial.print("Feuchtigkeit: ");
    Serial.print(feuchtigkeit);
    Serial.print(" %, ");
    Serial.print("Temperatur: ");
    Serial.print(temperatur);
    Serial.println(" C");
}
void wasserstandPruefen(bool &niveau) {
    if (niveau == true) {
        digitalWrite(gelbeLedPin, LOW);
    }
    else {
        digitalWrite(gelbeLedPin, HIGH);

        // Bis der Benutzer Wasser nachfüllt:
        while (!niveau) {
            piepen(); // 3x Piepsen aus dem Piezo-Summer

            // Ausgabe auf das LCD
            LCD.begin(16, 2);
            LCD.setCursor(0, 0);
            LCD.print("Kein Wasser");
            LCD.setCursor(0, 1);
            LCD.print("Warte 5 min...");

            // Ausgabe na die Konsole
            Serial.println("- Warte auf Wasserauffüllung (5 min)...");
            delay(wasserauffuellPruefungszeit);

            // Wasserstandsprüfung zum Beenden der while-Schleife
            Wasserstand = digitalRead(wasserstandsSensorPin);
            if (Wasserstand) {
                LCD.begin(16, 2);
                break;
            }
        }
    }
}


void pumpeEinschalten(bool &niveau) {
    if (niveau == true) {
        // Ausgabe auf das LCD
        LCD.setCursor(0, 1);
        LCD.print("                ");
        LCD.setCursor(0, 1);
        LCD.print("Bewässere...");

        // Einschalten der Pumpe
        digitalWrite(pumpePin, HIGH); // ein
        delay(pumpenlaufzeit);
        digitalWrite(pumpePin, LOW); // aus

        // Ausgabe auf das LCD
        LCD.begin(16, 2);
        LCD.setCursor(0, 0);
        LCD.print("Prüfe");
        LCD.setCursor(0, 1);
        LCD.print("Feuchtigkeit (3 min)");

        // Ausgabe na die Konsole
        Serial.println("+ Prüfe Feuchtigkeit...");
        delay(feuchtigkeitsPruefungszeit);
    }
    else {
        digitalWrite(pumpePin, LOW);
    }
}

void feuchtigkeitPruefen(USHORT &vlh) {
    if (vlh > minFeuchtigkeitsniveau) {
        // Feuchtigkeit ist OK
        digitalWrite(grueneLedPin, HIGH);
        digitalWrite(roteLedPin, LOW);
    }
    else {
        // Feuchtigkeit ist niedrig
        digitalWrite(grueneLedPin, LOW);
        digitalWrite(roteLedPin, HIGH);
        pumpeEinschalten(Wasserstand);
    }
}


void piepen() {
    for (USHORT i = 0; i < 3; i++) {
        digitalWrite(summerPin, HIGH);
        delay(450);
        digitalWrite(summerPin, LOW);
        delay(200);
    }
}
