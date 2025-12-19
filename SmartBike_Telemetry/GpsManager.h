#ifndef GPS_MANAGER_H
#define GPS_MANAGER_H
#include <TinyGPS++.h>

class GpsManager {
private:
    TinyGPSPlus gps;
public:
    double totalDistance = 0.0;
    double lastLat = 0.0, lastLon = 0.0;

    void init() { Serial1.begin(9600); }

    void update() {
        while(Serial1.available() > 0) gps.encode(Serial1.read());

        bool fixNow = gps.location.isValid() && gps.location.isUpdated() && gps.satellites.value() >= 4;
        if(!fixNow) return;

        double lat = gps.location.lat();
        double lon = gps.location.lng();

        if(lastLat == 0 && lastLon == 0) { lastLat=lat; lastLon=lon; return; }

        double dist = gps.distanceBetween(lat, lon, lastLat, lastLon);
        if(dist > 2.0 && dist < 100.0) {
            totalDistance += dist;
            lastLat = lat;
            lastLon = lon;
        }
    }

    double getLat() { return gps.location.isValid() ? gps.location.lat() : 0.0; }
    double getLon() { return gps.location.isValid() ? gps.location.lng() : 0.0; }
    double getAlt() { return gps.altitude.isValid() ? gps.altitude.meters() : 0.0; }
    double getSpeed() { return gps.speed.isValid() ? gps.speed.kmph() : 0.0; }
    int getSats() { return gps.satellites.isValid() ? gps.satellites.value() : 0; }
    double getOdometer() { return totalDistance / 1000.0; }
    double getAvgSpeed() {
        double hours = (millis()/3600000.0);
        return hours > 0 ? (totalDistance/1000.0)/hours : 0.0;
    }

    double distanceBetween(double lat1, double lon1, double lat2, double lon2) {
        const double R = 6371000.0; // meters
        double dLat = (lat2 - lat1) * 3.14159/180.0;
        double dLon = (lon2 - lon1) * 3.14159/180.0;
        double a = sin(dLat/2) * sin(dLat/2) +
                   cos(lat1*3.14159/180.0)*cos(lat2*3.14159/180.0) *
                   sin(dLon/2)*sin(dLon/2);
        double c = 2*atan2(sqrt(a), sqrt(1-a));
        return R*c;
    }
};

#endif
