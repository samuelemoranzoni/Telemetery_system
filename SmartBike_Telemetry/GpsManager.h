#ifndef GPS_MANAGER_H
#define GPS_MANAGER_H

#include <TinyGPS++.h>

class GpsManager {
  private:
    TinyGPSPlus gps; // The library that parses GPS text (NMEA)
    double totalDistance = 0.0; // Odometer (Total km)
    double lastLat = 0.0, lastLon = 0.0; // Previous position
    unsigned long startTime = 0; // When we started moving
    bool hasFix = false; // Do we have satellite signal?
    bool firstPoint = true; // Flag for the very first coordinate

  public:
    void init() {
      // Start serial communication with GPS module
      // Portenta uses Serial1 (Pin 13/14)
      Serial1.begin(9600); 
    }

    void update() {
      // Read all characters coming from the GPS wire
      while (Serial1.available() > 0) {
        gps.encode(Serial1.read());
      }

      // If we have a valid location signal...
      if (gps.location.isValid()) {
        if (!hasFix) {
          hasFix = true;
          startTime = millis(); // Start the timer for avg speed
        }

        if (firstPoint) {
          // Save starting point
          lastLat = gps.location.lat();
          lastLon = gps.location.lng();
          firstPoint = false;
        } else {
          // CALCULATE DISTANCE (Odometer)
          // Calculate meters between current point and previous point
          double dist = gps.distanceBetween(gps.location.lat(), gps.location.lng(), lastLat, lastLon);
          
          // Noise Filter: Only add distance if we moved more than 2 meters
          // (GPS drifts a bit even when standing still)
          if (dist > 2.0) {
            totalDistance += dist;
            lastLat = gps.location.lat();
            lastLon = gps.location.lng();
          }
        }
      }
    }

    // Standard Getters for raw GPS data
    bool isConnected() { return gps.location.isValid(); }
    double getLat() { return gps.location.lat(); }
    double getLon() { return gps.location.lng(); }
    double getAlt() { return gps.altitude.meters(); }
    double getSpeed() { return gps.speed.kmph(); }
    int getSats() { return gps.satellites.value(); }
    
    // CALCULATED METRIC: Odometer (in Km)
    double getOdometer() { return totalDistance / 1000.0; } 
    
    // CALCULATED METRIC: Average Speed (km/h)
    double getAvgSpeed() {
      if (totalDistance < 10 || !hasFix) return 0.0;
      // Convert millis to hours
      double hours = (millis() - startTime) / 3600000.0;
      if (hours <= 0) return 0.0;
      // Distance / Time
      return (totalDistance / 1000.0) / hours; 
    }

    // Helper to format time as "HH:MM:SS"
    String getTimeStr() {
      if (!gps.time.isValid()) return "--:--:--";
      char sz[10];
      sprintf(sz, "%02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());
      return String(sz);
    }
};
#endif