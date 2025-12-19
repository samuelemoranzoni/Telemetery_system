#include "mbed.h"
#include "BleManager.h"
#include "ImuManager.h"
#include "DisplayManager.h"
#include "GpsManager.h"
#include "NetworkManager.h"
#include <chrono> 

using namespace mbed;
using namespace rtos;
using namespace std::chrono;

// ---- WIFI CREDENTIALS ----
const char* WIFI_SSID = "Esu_Mob";
const char* WIFI_PASS = "091414145";

// ---- GLOBAL OBJECTS ----
BLEManager     ble;
ImuManager     imu;
GpsManager     gps;
DisplayManager display;
NetworkManager net(WIFI_SSID, WIFI_PASS);

// ---- THREADS ----
Thread bleThread(osPriorityNormal, 4096); 
Thread gpsThread(osPriorityNormal, 2048); 
Thread imuThread(osPriorityNormal, 2048); 
Thread netThread(osPriorityNormal, 8192); 
Thread displayThread(osPriorityLow, 4096);

volatile bool systemReady = false;
volatile bool bleReady = false;

// ----------------- TASK FUNCTIONS -----------------

void bleTask() {
    ble.begin();
    bleReady = true;

    while(true) {
        ble.update();
        ThisThread::sleep_for(milliseconds(50));
    }
}

void gpsTask() {
    while(!systemReady || !bleReady) ThisThread::sleep_for(milliseconds(100));
    while(true) {
        gps.update();
        ThisThread::sleep_for(milliseconds(50));
    }
}

void imuTask() {
    while(!systemReady || !bleReady) ThisThread::sleep_for(milliseconds(100));
    while(true) {
        imu.update();
        ThisThread::sleep_for(milliseconds(20));
    }
}

void netTask() {
    while(!systemReady || !bleReady) ThisThread::sleep_for(milliseconds(100));

    double lastSpeed = 0.0;
    double lastAlt   = 0.0;
    unsigned long lastTime = millis();

    while(true) {
        if (ble.isReady() && WiFi.status() != WL_CONNECTED) net.init();

        // --- READ SENSORS ---
        int bpm      = ble.getHeartRate();
        float gf     = imu.getGForce();
        float slope  = imu.getSlope();
        float lean   = imu.getLean();
        float vib    = imu.getVibration();
        bool crash   = imu.isCrash();
        float temp   = imu.getTemp();
        double lat   = gps.getLat();
        double lon   = gps.getLon();
        double alt   = gps.getAlt();
        double spd   = gps.getSpeed();          // km/h
        double odo   = gps.getOdometer();
        double avgSpd= gps.getAvgSpeed();
        int sats     = gps.getSats();
        bool wifiOk  = (WiFi.status() == WL_CONNECTED);
        bool bleOk   = ble.isDeviceConnected();

        // --- NEW CALCULATIONS ---
        unsigned long now = millis();
        double dt = (now - lastTime) / 1000.0; // seconds
        lastTime = now;

        double speed_mps = spd / 3.6;
        double lastSpeed_mps = lastSpeed / 3.6;

        // Acceleration
        double acceleration = (speed_mps - lastSpeed_mps) / dt; // m/s²
        lastSpeed = spd;

        // Climb angle from GPS altitude
        double distance = gps.distanceBetween(lat, lon, gps.lastLat, gps.lastLon);
        double altDiff = alt - lastAlt;
        double climbAngle = 0.0;
        if(distance > 0.1) climbAngle = atan2(altDiff, distance) * 57.29;
        lastAlt = alt;

        // Lateral G-force in turns
        float lateralG = gf * sin(lean * 3.14159 / 180.0);

        // Athlete effort indicator
        float effort = bpm / (spd + 0.1);

        // --- TELEMETRY OUTPUT ---
        Serial.print("SPD: "); Serial.print(spd);
        Serial.print(", ACC: "); Serial.print(acceleration, 2);
        Serial.print(", CLIMB: "); Serial.print(climbAngle, 2);
        Serial.print(", LAT_G: "); Serial.print(lateralG, 2);
        Serial.print(", EFFORT: "); Serial.println(effort, 2);

        // Send telemetry
        if(wifiOk) {
            net.sendTelemetry(bpm, gf, slope, lean, vib, crash, temp,
                              lat, lon, alt, spd, odo, avgSpd,
                              acceleration, climbAngle, lateralG, effort);
        }

        ThisThread::sleep_for(milliseconds(1000));
    }
}

void displayTask() {
    while(!systemReady || !bleReady) ThisThread::sleep_for(milliseconds(100));
    while(true) {
        // Read sensors
        int bpm      = ble.getHeartRate();
        double spd   = gps.getSpeed();
        float gf     = imu.getGForce();
        float lean   = imu.getLean();
        bool crash   = imu.isCrash();
        bool wifiOk  = (WiFi.status() == WL_CONNECTED);
        bool bleOk   = ble.isDeviceConnected();

        static double lastSpeed = 0.0;
        static double lastAlt = 0.0;
        static unsigned long lastTime = millis();
        unsigned long now = millis();
        double dt = (now - lastTime) / 1000.0;
        lastTime = now;

        double speed_mps = spd / 3.6;
        double lastSpeed_mps = lastSpeed / 3.6;
        double acceleration = (speed_mps - lastSpeed_mps) / dt;
        lastSpeed = spd;

        double climbAngle = 0.0;
        double distance = gps.distanceBetween(gps.getLat(), gps.getLon(), gps.lastLat, gps.lastLon);
        double altDiff = gps.getAlt() - lastAlt;
        if(distance > 0.1) climbAngle = atan2(altDiff, distance) * 57.29;
        lastAlt = gps.getAlt();

        float lateralG = gf * sin(lean * 3.14159 / 180.0);

        display.drawTelemetry(bpm, spd, acceleration, lateralG, climbAngle, wifiOk, bleOk, crash);

        ThisThread::sleep_for(milliseconds(500));
    }
}

// ----------------------- SETUP -----------------------

void setup() {
    pinMode(LEDR, OUTPUT);
    pinMode(LEDG, OUTPUT);
    pinMode(LEDB, OUTPUT);
    digitalWrite(LEDR, LOW);

    Serial.begin(115200);
    unsigned long start = millis();
    while (!Serial && millis() - start < 5000);

    Serial.println("\n--- SMART BIKE STARTUP ---");

    display.init();
    imu.init();
    gps.init();

    // Start threads
    bleThread.start(bleTask);
    gpsThread.start(gpsTask);
    imuThread.start(imuTask);
    netThread.start(netTask);
    displayThread.start(displayTask);

    systemReady = true;

    digitalWrite(LEDR, HIGH);
    digitalWrite(LEDG, LOW);
    Serial.println("--- ALL SYSTEMS ACTIVE ---");
}

void loop() {
    delay(1000);
}
