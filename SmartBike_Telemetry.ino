/*
 * SMART BIKE TELEMETRY - ULTIMATE EDITION
 * Hardware: Portenta H7 + PCBoard + GPS + IMU + OLED + Polar OH1
 */

#include "BleManager.h"
#include "ImuManager.h"
#include "DisplayManager.h"
#include "GpsManager.h"     // <--- Nuovo amico
#include "NetworkManager.h"

// --- CREDENZIALI ---
const char* WIFI_SSID = "Esu_Mob"; // Il tuo hotspot senza spazi
const char* WIFI_PASS = "09141414";

// --- OGGETTI ---
BleManager ble;
ImuManager imu;
GpsManager gps;
DisplayManager display;
NetworkManager net(WIFI_SSID, WIFI_PASS);

unsigned long lastSend = 0;

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("--- SYSTEM BOOT ---");

  display.init();
  imu.init();
  gps.init();  // Avvia la seriale GPS
  net.init();  // Connette WiFi
  ble.init();  // Cerca Polar
}

void loop() {
  // 1. Aggiornamento continuo dei sensori
  gps.update(); // FONDAMENTALE chiamarlo spesso per leggere la seriale
  imu.update();
  ble.update();
  net.update();

  // 2. Logica di Invio (1 Hz - Ogni secondo)
  if (millis() - lastSend > 1000) {
    lastSend = millis();
    
    // Raccogli TUTTI i dati
    int bpm = ble.getBpm();
    
    // Dati IMU
    float gf = imu.getGForce();
    float sl = imu.getSlope();
    float ln = imu.getLean();
    float vb = imu.getVibration();
    bool cr = imu.isCrash();
    float tm = imu.getTemp();

    // Dati GPS
    double lat = gps.getLat();
    double lon = gps.getLon();
    double alt = gps.getAlt();
    double spd = gps.getSpeed();
    double odo = gps.getOdometer();
    double avg = gps.getAvgSpeed();
    int sats = gps.getSats();

    // Aggiorna Display Fisico
    display.drawTelemetry(bpm, gf, spd, sats, (WiFi.status() == WL_CONNECTED), ble.isConnected());

    // Manda al Cloud
    net.sendTelemetry(bpm, gf, sl, ln, vb, cr, tm, lat, lon, alt, spd, odo, avg);
  }
}