#ifndef GPS_MANAGER_H
#define GPS_MANAGER_H

#include <TinyGPS++.h>

class GpsManager {
  private:
    TinyGPSPlus gps;
    double totalDistance = 0.0;
    double lastLat = 0.0, lastLon = 0.0;
    unsigned long startTime = 0;
    bool hasFix = false;
    bool firstPoint = true;

  public:
    void init() {
      // Sulla Portenta e Breakout, Serial1 è la porta UART principale
      Serial1.begin(9600); 
    }

    void update() {
      while (Serial1.available() > 0) {
        gps.encode(Serial1.read());
      }

      // Logica Odometro (Calcolo Distanza)
      if (gps.location.isValid()) {
        if (!hasFix) {
          hasFix = true;
          startTime = millis(); // Start cronometro al primo fix
        }

        if (firstPoint) {
          lastLat = gps.location.lat();
          lastLon = gps.location.lng();
          firstPoint = false;
        } else {
          // Calcola distanza in metri dal punto precedente
          double dist = gps.distanceBetween(gps.location.lat(), gps.location.lng(), lastLat, lastLon);
          
          // Filtro "Noise": aggiorna solo se ci siamo mossi di almeno 2 metri
          if (dist > 2.0) {
            totalDistance += dist;
            lastLat = gps.location.lat();
            lastLon = gps.location.lng();
          }
        }
      }
    }

    // --- GETTERS ---
    bool isConnected() { return gps.location.isValid(); }
    double getLat() { return gps.location.lat(); }
    double getLon() { return gps.location.lng(); }
    double getAlt() { return gps.altitude.meters(); }
    double getSpeed() { return gps.speed.kmph(); }
    int getSats() { return gps.satellites.value(); }
    
    // Distanza in Km
    double getOdometer() { return totalDistance / 1000.0; } 
    
    // Velocità media in km/h
    double getAvgSpeed() {
      if (totalDistance < 10 || !hasFix) return 0.0;
      double hours = (millis() - startTime) / 3600000.0;
      if (hours <= 0) return 0.0;
      return (totalDistance / 1000.0) / hours; 
    }

    String getTimeStr() {
      if (!gps.time.isValid()) return "--:--:--";
      char sz[10];
      sprintf(sz, "%02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());
      return String(sz);
    }
};
#endif