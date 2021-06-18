/*  Copyright (C)  2018  Frank Schuetz.
    Permission is granted to copy, distribute and/or modify this document
    under the terms of the GNU Free Documentation License, Version 1.3
    or any later version published by the Free Software Foundation;
    with no Invariant Sections, no Front-Cover Texts, and no Back-Cover Texts.
    A copy of the license is included in the section entitled "GNU
    Free Documentation License".
 * 
 * Sketch ESPLog - SO Datenlogger für Solaranlagen
 * Erstellt Februar/März 2018 von Frank Schütz 
 * www.frank-schuetz.de / post@frank-schuetz.de
 * 
 * Hinweiss: Alle Pin's außer GPIO 16 sind Interrupteingänge
 * 
 * // Struktur zum Speichern der gesammten ertragsabhängigen Variablen
 *
 * 0 => Tagesertrag x.xx kWh
 * 1 => Monatsertrag x.xx kWh
 * 2 => Jahresertrag x.xx kWh
 * 3 => Spezifisch x.xx kWh/kWp
 * 4 => Vergütung x.xx €
 * 5 => Maximal x W
 * 6 => Einspeiseleistung x W
 * 7 => Vcc x.xx V
 * 8 => RSSI -x db
 * 9 => upTime min
 * 10 => Zählerstand kWh
 * 
 * id => Index des Wertes
 * value => Unformatierter Wert Wh
 * printValue => Formatierter Wert kWh
 * realName => Angezeigter Name
 * unit => Angezeigte Einheit
*/
 
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <EasyNTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <TimeLib.h>
#include <EEPROM.h>
//#include <ArduinoJson.h>
#include <FS.h>

extern "C" {
#include "user_interface.h"
}

// Debugausgaben kompilieren (1) oder nicht (0)
#define DEBUG 0

#if (DEBUG == 0)
  #define sp(...)
  #define spn(...)
  #define sbeg(...)
#else
  #define sp(...) Serial.print(__VA_ARGS__)
  #define spn(...) Serial.println(__VA_ARGS__)
  #define sbeg(...) Serial.begin(__VA_ARGS__); while(!Serial)
#endif


#define INT_PIN1 5      // Interruptpin für den S0 GPIO5 / D1 
#define LED_PIN1 14     // LED GPIO 14 / D5

#define anzahlTage 30   // Maximale Tage im Monat -1
#define anzahlMonate 11 // maximale Monate im Jahr -1

#define adresseEepromZK 1488 // Zählerkonstante
#define adresseEepromSM 1492 // Intervall Simulation
#define adresseEepromCO 1496 // Vergütung in cent/10000
#define adresseEepromGL 1500 // Generatorleistung
#define adresseEepromFhemHost 1504 // Controller Adresse
#define adresseEepromFhemPort 1512 // Controller Port
#define adresseEepromOF 1514 // Zähler Offset

// Netzwerk Vorgaben
const char* host = "ESPLog";
const char *ssid[] = {"SSID1","SSID2"};
const char *password[] = {"Passwd1","Passwd2"};

// FHEM Netzwerk Host
byte fhemHost[] = {0,0,0,0};
int fhemPort = 8383;

// HTTP Authentifizierung Setup / SPIFFS / Update
const char* www_username = "user";
const char* www_password = "Passwd";

// vars
unsigned int interval = 2000; // Sekunden Counter

//unsigned int currentMillis;
unsigned int previousMillis = 0;
//unsigned int previousMillis2 = 0;
unsigned int waitTime = 0;
unsigned long upTime; // in minuten
unsigned t; //unixtime

unsigned int counter;                                     // Counter für eingehende Impulse
unsigned int fuenfMinutenZaehler[288];                    // Array der Fünfminutenwerte eines Tages
unsigned int tagesCounter[anzahlMonate+1][anzahlTage+1];  // Array für Tageswerte eines Jahres in Wh

unsigned int previousZaehlerWh;
unsigned int impulseProKwh = 1000; // Standardvorgabe
float compensation; // Vergütung
unsigned int generatorLeistung = 1; // Generatorleistung in Wp
float offset = 0; // Zähleroffset

bool sommerzeit;
bool tickDetected = false;

const int lenStore = 11;
struct valueStore
{
  byte id;
  float value = 0;
  char printValue[15] = "0";
  char realName[20] = "";
  char unit[15] = "";
} ausgabe[lenStore]; 

// Struktur zum Speichern des Datums von Heute/Vortag
struct Date
{
   byte day;
   byte month;
   int year;
};
Date yesterday;
Date today;

WiFiUDP udp;

EasyNTPClient ntpClient(udp, "de.pool.ntp.org", ((1*60*60))); // IST = GMT + 1:00
time_t syncProvider()
{
return ntpClient.getUnixTime();
}
 
ESP8266WebServer server(80);  // Serverport  hier einstellen
ESP8266HTTPUpdateServer httpUpdater;
File fsUploadFile;                      //Hält den aktuellen Upload

ADC_MODE(ADC_VCC);

#if (DEBUG == 1) 
  os_timer_t Timer1;
#endif

// ########################################################################################
// ##### ISR ##############################################################################
// ########################################################################################
#if (DEBUG == 1) 
void timerCallback(void *pArg) { 
  tickDetected = true;
  *((int *) pArg) += 1;
} 
#endif

void interruptRoutineFalling() {
  tickDetected = true;
  counter++;
}

// ########################################################################################
// ##### S E T U P ######################################################################## 
// ########################################################################################

void setup()
{ 
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW); // einschalten

#if (DEBUG == 1) 
  Serial.begin(115200);
  delay(10);
  Serial.println("Booting ...");
#endif

  pinMode(LED_PIN1, OUTPUT);
  pinMode(INT_PIN1, INPUT_PULLUP);

  setupFS ();
  setupWifi ();
  setupNtp ();
  setupStructAusgabe ();
  setupEeprom ();
  setupHttpServer ();

  write2logfile("ESPLog gestartet");
  
  httpUpdater.setup(&server, "/update", www_username, www_password);
  server.begin();                     //  Starte den Server
  
  spn("HTTPUpdateServer ready! Open http://" + String(host) + ".local/update in your browser\n");
  
  // GPIO für Impulse einstellen und Interrupts starten ######################
  pinMode(INT_PIN1, INPUT_PULLUP);
  attachInterrupt(INT_PIN1, interruptRoutineFalling, FALLING);

#if (DEBUG == 1) 
  os_timer_setfn(&Timer1, timerCallback, &counter);
  os_timer_arm(&Timer1, interval, true);
#endif

  spn("Ready");
}




