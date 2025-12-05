/*
 * SMART BIKE TELEMETRY - ULTIMATE EDITION
 * Hardware: Portenta H7 + PCBoard + GPS + IMU + OLED + Polar OH1
 */

#include "BleManager.h"
#include "ImuManager.h"
#include "DisplayManager.h"
#include "GpsManager.h"     
#include "NetworkManager.h"

// 
const char* WIFI_SSID = "Adrian"; // hotspot without spaces
const char* WIFI_PASS = "12345678";

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
  gps.init();  // Start the serial GPS
  net.init();  // Connect to WiFi
  ble.init();  // Look for Polar
  Serial.println("PUTA");
}

void loop() {
  // 1. Update of sensors
  gps.update(); // FONDAMENTALE chiamarlo spesso per leggere la seriale
  imu.update();
  ble.update();
  net.update();

  // 2. Logic (1 Hz - every second)
  if (millis() - lastSend > 1000) {
    lastSend = millis();
    
    // bpm
    int bpm = ble.getBpm();
    
    // Data IMU
    float gf = imu.getGForce();
    float sl = imu.getSlope();
    float ln = imu.getLean();
    float vb = imu.getVibration();
    bool cr = imu.isCrash();
    float tm = imu.getTemp();

    // Data GPS
    double lat = gps.getLat();
    double lon = gps.getLon();
    double alt = gps.getAlt();
    double spd = gps.getSpeed();
    double odo = gps.getOdometer();
    double avg = gps.getAvgSpeed();
    int sats = gps.getSats();

    // Update Display 
    display.drawTelemetry(bpm, gf, spd, sats, (WiFi.status() == WL_CONNECTED), ble.isConnected());

    // Send to cloud ( to broker and after subscriber such us Mqttpanel can visualize data)
    net.sendTelemetry(bpm, gf, sl, ln, vb, cr, tm, lat, lon, alt, spd, odo, avg);
  }
}