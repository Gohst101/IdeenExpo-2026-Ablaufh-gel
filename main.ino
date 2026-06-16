/*
Dieses Script ist dazu da um die Ablaufhügel zu kontrollieren.

Funktion:
1. Erkenne ob ein Zug vor dem ersten sensor ist.
2. Holle alle Werte aus den Sensoren und werte diese aus.
3. Schalte die richtige Weiche in die Richtige richtung.
4. Gebe informationen auf dem Display aus.

Für später:
-> evaluation(); // Auswertung der Daten wenn Sensor 0 Weiß erkennt
-> trigger_switch(); // trigger_switch(pin) schaltet den pin an und aus
-> updateDisplay(); // Aktuallisiert das Display

Dinge die noch fehlen oder Falsch sind:
- Problem: Wenn ein Wagong schneller ist als sonst oder Sensor 0 zu spät weiß erkennt dann verändern sich die werte von Sensor 1 und Sensor 2

Auswertungen:
Sensor 0 dient zur erkennung wann ausgewertet werden soll.
An + An = Weiche 1
An + Aus = Weiche 2
Aus + Aus = Weiche 3
Aus + AN = Weiche 4

*/

// https://cdn-reichelt.de/documents/datenblatt/A500/CNY70_VISHAY.pdf

#include <Arduino.h>
#include <U8g2lib.h>

U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0);

// Weichen
#define switch_1_R 2
#define switch_1_L 3
#define switch_2_R 4
#define switch_2_L 5
#define switch_3_R 6
#define switch_3_L 7

// Sensoren
#define sensor_0 15
#define sensor_1 14
#define sensor_2 16

// Bildschirm Texte
char sensor_1_text[8] = "/";
char sensor_2_text[8] = "/";
char sensor_0_text[8] = "/";

// Variablen für den Code
int delaySwitchPowerTime = 20; // 20 Milisekunden
int mittelwert = 500; // Mittelwert für Schwarz - Weiß (0 - 1023)
int sensor_0_wert = 0; // Sensor 0
int sensor_1_wert = 0; // Sensor 1
int sensor_2_wert = 0; // Sensor 2

int rail_arrow = 37; // 28 - 37 - 46 - 55 => Für den Bildschirm Pfeil welcher die Strecke anzeigt

int pause_time = 2000; // 2000 Milisekunden Pause nachdem ein Wagong erkannt wurde (Weichen Relay Schutz)
unsigned long latest_trigger; // Letzter Trigger
int updateDisplayTime = 1; // Update Display
unsigned long latestDisplayUpdate; // Letztes Display Update
bool triggerActive = false; // Trigger an oder aus
bool exclamationState = false; // Ausrufezeichen Status

int testing_switch_delay = 1000; // Pausen nach dem Schalten einer Weiche beim Start Test

// Bitmaps
static const unsigned char image_Aktion_erkannt_bits[] = { 0x03, 0x03, 0x03, 0x03, 0x03, 0x00, 0x03, 0x03 };
static const unsigned char image_Pfeile_bits[] = { 0x04, 0x08, 0x1f, 0x08, 0x04, 0x00, 0x00, 0x04, 0x08, 0x1f, 0x08, 0x04, 0x00, 0x00, 0x04, 0x08, 0x1f, 0x08, 0x04 };
static const unsigned char image_Strecke_bits[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x49, 0x92, 0x24, 0x49, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2b, 0x49, 0x92, 0x24, 0x49, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x66, 0x49, 0x92, 0x24, 0x49, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x4d, 0x49, 0x92, 0x24, 0x49, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xd8, 0xff, 0xff, 0xff, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa8, 0x31, 0x49, 0x92, 0x24, 0x49, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x2b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x49, 0xc6, 0x24, 0x49, 0x92, 0x24, 0x49, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x2b, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24, 0x49, 0x06, 0x00, 0x00, 0x00, 0x00, 0x66, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24, 0x49, 0x06, 0x00, 0x00, 0x00, 0x60, 0x4d, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24, 0x49, 0x06, 0x00, 0x00, 0x00, 0xc0, 0xd8, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x07, 0x00, 0x00, 0x00, 0xa8, 0x31, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24, 0x49, 0x00, 0x00, 0x00, 0x00, 0x30, 0x2b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x92, 0x24, 0x49, 0xc6, 0x24, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24, 0x49, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x07, 0x92, 0x24, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24, 0x49, 0x06, 0x92, 0x24, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24, 0x49, 0x06, 0x92, 0x24, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24, 0x49, 0x06, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x07, 0x92, 0x24, 0x49, 0x92, 0x24, 0x49, 0xc6, 0x24, 0x49, 0x92, 0x24, 0x49, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x2b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa8, 0x31, 0x49, 0x92, 0x24, 0x49, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xd8, 0xff, 0xff, 0xff, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x4d, 0x49, 0x92, 0x24, 0x49, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x66, 0x49, 0x92, 0x24, 0x49, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2b, 0x49, 0x92, 0x24, 0x49, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x49, 0x92, 0x24, 0x49, 0x00 };
static const unsigned char image_Zeil_Pfeil_bits[] = { 0x04, 0x06, 0x07, 0x06, 0x04 };
static const unsigned char image_Sensor_Optisch_bits[] = { 0x7f, 0x55, 0x7f, 0x7f, 0x63, 0x63 };


void setup() {
  Serial.begin(115200);
  u8g2.begin();

  // Pin Modes
  // Weichen
  pinMode(switch_1_R, OUTPUT);
  pinMode(switch_1_L, OUTPUT);
  pinMode(switch_2_R, OUTPUT);
  pinMode(switch_2_L, OUTPUT);
  pinMode(switch_3_R, OUTPUT);
  pinMode(switch_3_L, OUTPUT);
  // Sensoren
  pinMode(sensor_0, INPUT);
  pinMode(sensor_1, INPUT);
  pinMode(sensor_2, INPUT);

  // Prints
  Serial.println("");
  Serial.println("");
  Serial.println("====================");
  Serial.println("  Starting Arduino  ");
  Serial.println("====================");
  Serial.println("");
  Serial.println("");
  Serial.println("");
  checkRailSwitchs(); // Checking Rail switches
  Serial.println("Waiting for Sensor 0 to get an Signal..");
}

void loop() {
  // Schaut ob Sensor 0 einen Wert hat.
  // INFO: Benötigt einen weg während der Auswertung diesen Teil zu stoppen. => triggerActive ✅
  sensor_0_wert = analogRead(sensor_0);
  /*
  sensor_1_wert = analogRead(sensor_1);
  sensor_2_wert = analogRead(sensor_2);

  Serial.print("S1=");
  Serial.print(sensor_1_wert);
  Serial.print(" | S2=");
  Serial.print(sensor_2_wert);
  Serial.print(" | S0=");
  Serial.println(sensor_0_wert);
  */

  // Schaut ob Sensor 0 einen hohen wert hat => Hoher Wert = Weiß
  if (sensor_0_wert > 500 && !triggerActive) {
    // Werte Auslesen
    sensor_1_wert = analogRead(sensor_1);
    sensor_2_wert = analogRead(sensor_2);

    triggerActive = true;
    exclamationState = true;
    latest_trigger = millis();

/*
    Serial.println("Sensor 0");
    Serial.println(analogRead(sensor_0));
    Serial.println("Sensor 1");
    Serial.println(analogRead(sensor_1));
    Serial.println("Sensor 2");
    Serial.println(analogRead(sensor_2));
*/
    evaluation();
  }

  // Reset des triggerActives nach einiger Zeit
  if (triggerActive && millis() - latest_trigger > pause_time) {
    triggerActive = false;
    exclamationState = false;
  }

  if (millis() - latestDisplayUpdate >= updateDisplayTime) {
    updateDisplay();
    latestDisplayUpdate = millis();
  }
}

// Auswertung
void evaluation() {
  // Serial Output
  Serial.println("");
  Serial.println("");
  Serial.println("====================");
  Serial.println("     Auswertung     ");
  Serial.println("====================");
  Serial.println("");
  Serial.println("Sensor 1:");
  Serial.println(sensor_1_wert);
  Serial.println("Sensor 2:");
  Serial.println(sensor_2_wert);
  Serial.println("==============");
  Serial.println("Auswertung:");
  /* INFO
    Gegebenenfalls muss man 2 mal die trigger_switch funktion einbauen
    damit eben ein Wagon durch 2 richtig gestellte Weichen fahren kann.
    Diesen Teil muss man eben anpassen.

    < = Kleiner als
    > = Größer als

  */
  // Sensordaten auswerten
  if (sensor_1_wert > mittelwert && sensor_2_wert > mittelwert) {  // HIGH - HIGH || Weiß - Weiß
    Serial.println("INFO - Weiß + Weiß - Option 1");
    strcpy(sensor_0_text, "Erkannt");
    strcpy(sensor_1_text, "Weiss");
    strcpy(sensor_2_text, "Weiss");
    rail_arrow = 28;
    trigger_switch(switch_1_L);
    trigger_switch(switch_2_L);
    return;
  } else if (sensor_1_wert > mittelwert && sensor_2_wert < mittelwert) {  // HIGH - LOW || Weiß - Schwarz
    Serial.println("INFO - Weiß + Schwarz - Option 2");
    strcpy(sensor_0_text, "Erkannt");
    strcpy(sensor_1_text, "Weiss");
    strcpy(sensor_2_text, "Schwarz");
    rail_arrow = 37;
    trigger_switch(switch_1_L);
    trigger_switch(switch_2_R);
    return;
  } else if (sensor_1_wert < mittelwert && sensor_2_wert < mittelwert) {  // LOW - LOW || Schwarz - Schwarz
    Serial.println("INFO - Schwarz + Schwarz - Option 3");
    strcpy(sensor_0_text, "Erkannt");
    strcpy(sensor_1_text, "Schwarz");
    strcpy(sensor_2_text, "Schwarz");
    rail_arrow = 46;
    trigger_switch(switch_1_R);
    trigger_switch(switch_3_R);
    return;
  } else if (sensor_1_wert < mittelwert && sensor_2_wert > mittelwert) {  // LOW - HIGH || Schwarz - Weiß
    Serial.println("INFO - Schwarz + Weiß - Option 4");
    strcpy(sensor_0_text, "Erkannt");
    strcpy(sensor_1_text, "Schwarz");
    strcpy(sensor_2_text, "Weiss");
    rail_arrow = 55;
    trigger_switch(switch_1_R);
    trigger_switch(switch_3_L);
    return;
  } else {
    Serial.println("ERROR - Es konnte zu den Sensordaten keine Weichenkombination gefunden werden.");
    strcpy(sensor_0_text, "ERROR");
    strcpy(sensor_1_text, "/");
    strcpy(sensor_2_text, "/");
    return;
  }
}

// Weichen Toggle
void trigger_switch(int pin) {
  Serial.println("");
  Serial.println("");
  Serial.println("====================");
  Serial.println("   Weiche schalten  ");
  Serial.println("====================");
  Serial.println("");

  digitalWrite(pin, HIGH);
  delay(delaySwitchPowerTime);
  digitalWrite(pin, LOW);

  Serial.println("INFO - Weiche wurde erfolgreich geschaltet - Pin:");
  Serial.println(pin);
  Serial.println("");
}

// Check Rail Switches
// Wird am anfang ausgeführt
void checkRailSwitchs() {
  Serial.println("====================");
  Serial.println("   Switch Check..   ");
  Serial.println("====================");
  Serial.println("");
  Serial.println("");
  Serial.println("");
  trigger_switch(switch_1_R);
  delay(testing_switch_delay);
  trigger_switch(switch_1_L);
  delay(testing_switch_delay);
  trigger_switch(switch_1_R);
  delay(testing_switch_delay);
  trigger_switch(switch_2_R);
  delay(testing_switch_delay);
  trigger_switch(switch_2_L);
  delay(testing_switch_delay);
  trigger_switch(switch_2_R);
  delay(testing_switch_delay);
  trigger_switch(switch_3_R);
  delay(testing_switch_delay);
  trigger_switch(switch_3_L);
  delay(testing_switch_delay);
  trigger_switch(switch_3_R);
  delay(testing_switch_delay);
  Serial.println("====================");
  Serial.println("  Switches checked  ");
  Serial.println("====================");
}

// Display Updaten
void updateDisplay() {

  u8g2.firstPage();
  do {

    u8g2.setFontMode(1);
    u8g2.setBitmapMode(1);

    // Outline
    u8g2.drawLine(1, 63, 126, 63);
    u8g2.drawLine(0, 0, 127, 0);
    u8g2.drawLine(127, 1, 127, 63);
    u8g2.drawLine(0, 1, 0, 63);
    u8g2.drawLine(94, 1, 94, 22);
    u8g2.drawLine(1, 23, 126, 23);

    u8g2.drawXBM(103, rail_arrow, 3, 5, image_Zeil_Pfeil_bits);
    u8g2.drawXBM(2, 27, 99, 34, image_Strecke_bits);

    u8g2.setFont(u8g2_font_4x6_tr);

    // Update Text if Active
    if (triggerActive) {
      u8g2.drawStr(64, 7, sensor_0_text);
      u8g2.drawStr(64, 14, sensor_1_text);
      u8g2.drawStr(64, 21, sensor_2_text);
    }

    // Always Update Numbers
    char buf[8];

    u8g2.drawStr(2, 21, "Sensor 2:");
    u8g2.drawStr(2, 7, "Sensor 0:");
    u8g2.drawStr(2, 14, "Sensor 1:");


    itoa(sensor_2_wert, buf, 10);
    u8g2.drawStr(38, 21, buf);

    itoa(sensor_1_wert, buf, 10);
    u8g2.drawStr(38, 14, buf);

    itoa(sensor_0_wert, buf, 10);
    u8g2.drawStr(38, 7, buf);

    u8g2.drawXBM(12, 37, 7, 6, image_Sensor_Optisch_bits);
    u8g2.drawXBM(56, 2, 5, 19, image_Pfeile_bits);

    // Ausrufezeichen anzeigen / nicht anzeigen
    if (exclamationState) {
      u8g2.drawXBM(20, 34, 2, 8, image_Aktion_erkannt_bits);
    }

  } while (u8g2.nextPage());
}
