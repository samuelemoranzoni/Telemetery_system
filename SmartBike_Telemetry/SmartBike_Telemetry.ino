/*
 * SMART BIKE TELEMETRY - HARDWARE DIAGNOSTIC MODE
 * Status: DEBUGGING "SILENT FREEZE"
 */

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
const char* WIFI_PASS = "09141414";

// ---- GLOBAL OBJECTS ----
BLEManager     ble;
ImuManager     imu;
GpsManager     gps;
DisplayManager display;
NetworkManager net(WIFI_SSID, WIFI_PASS);

// ---- THREADS ----
// Reduced stack slightly to ensure we fit in RAM, but kept high enough for stability
Thread bleThread(osPriorityHigh, 4096); 
Thread gpsThread(osPriorityNormal, 2048); 
Thread imuThread(osPriorityNormal, 2048); 
Thread netThread(osPriorityNormal, 6000); 
Thread displayThread(osPriorityLow, 4096);

// ---- GLOBAL FLAGS ----
volatile bool systemReady = false; // Prevents threads from reading sensors before init

// ----------------- TASK FUNCTIONS -----------------

void bleTask() {
    while(!systemReady) ThisThread::sleep_for(milliseconds(100)); // Wait for setup
    while (true) {
        ble.update();
        ThisThread::sleep_for(milliseconds(20));
    }
}

void gpsTask() {
    while(!systemReady) ThisThread::sleep_for(milliseconds(100));
    while (true) {
        gps.update();
        ThisThread::sleep_for(milliseconds(10));
    }
}

void imuTask() {
    while(!systemReady) ThisThread::sleep_for(milliseconds(100));
    while (true) {
        imu.update();
        ThisThread::sleep_for(milliseconds(20));
    }
}

void netTask() {
    // 1. Wait for system to fully boot
    while(!systemReady) ThisThread::sleep_for(milliseconds(100));
    
    while (true) {
        // 2. Check Connection Status
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[Net] WiFi Disconnected/Failed. Retrying...");
            
            // This function blocks for 10 seconds trying to connect.
            // Since we are in a thread, this is SAFE and won't freeze the display!
            net.init(); 

            // If it still failed, wait 5 seconds before trying again
            // This prevents spamming the router if it's down.
            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("[Net] Still no WiFi. Waiting 5s...");
                ThisThread::sleep_for(milliseconds(5000));
                continue; // Jump back to the start of the loop
            }
        }

        // 3. If we are here, WiFi is Connected!
        net.update(); // Keep MQTT alive
        
        static unsigned long lastSendTime = 0;
        if (millis() - lastSendTime > 1000) {
            // Gather Data
            int bpm     = ble.getHeartRate(); 
            float gf    = imu.getGForce();
            float slope = imu.getSlope();
            float lean  = imu.getLean();
            float vib   = imu.getVibration();
            bool crash  = imu.isCrash();
            float temp  = imu.getTemp();
            double lat  = gps.getLat();
            double lon  = gps.getLon();
            double alt  = gps.getAlt();
            double spd  = gps.getSpeed();
            double odo  = gps.getOdometer();
            double avg  = gps.getAvgSpeed();

            // Send to Cloud
            net.sendTelemetry(bpm, gf, slope, lean, vib, crash, temp, lat, lon, alt, spd, odo, avg);
            
            lastSendTime = millis();
        }
        
        ThisThread::sleep_for(milliseconds(50));
    }
}

void displayTask() {
    while(!systemReady) ThisThread::sleep_for(milliseconds(100));

    while (true) {
        int bpm    = ble.getHeartRate();      
        bool bleOK = ble.isDeviceConnected(); 
        float gf   = imu.getGForce();
        double sp  = gps.getSpeed();
        int sats   = gps.getSats();
        
        // SAFE WIFI CHECK: Only check if network manager has likely started
        // Directly accessing WiFi.status() before WiFi.begin() can crash Mbed
        bool wifiOK = false;
        if (millis() > 5000) { // Assume WiFi init starts after 5 seconds
             wifiOK = (WiFi.status() == WL_CONNECTED);
        }

        display.drawTelemetry(bpm, gf, sp, sats, wifiOK, bleOK);
        ThisThread::sleep_for(milliseconds(100)); 
    }
}

// ----------------------- HELPER: LED STATUS -----------------------
void setStatusColor(bool r, bool g, bool b) {
  digitalWrite(LEDR, r ? LOW : HIGH); // LOW is ON for Portenta
  digitalWrite(LEDG, g ? LOW : HIGH);
  digitalWrite(LEDB, b ? LOW : HIGH);
}

// ----------------------- SETUP -----------------------

void setup() {
    // 1. HARDWARE PROOF OF LIFE (RED LED)
    pinMode(LEDR, OUTPUT);
    pinMode(LEDG, OUTPUT);
    pinMode(LEDB, OUTPUT);
    setStatusColor(true, false, false); // RED = Booting
    
    Serial.begin(115200);
    
    // 2. FORCE WAIT FOR SERIAL (Max 10 seconds)
    // This ensures you don't miss the start messages
    long timeout = millis() + 10000;
    while (!Serial && millis() < timeout) {
        setStatusColor(true, false, true); // PURPLE = Waiting for USB
        delay(100);
    }

    Serial.println("\n\n--- DIAGNOSTIC BOOT START ---");
    setStatusColor(true, true, false); // ORANGE = Initializing Sensors

    // 3. INIT HARDWARE (Staggered to prevent power spike)
    Serial.print("[1/5] Display... ");
    display.init();
    Serial.println("OK");
    delay(200);

    Serial.print("[2/5] IMU... ");
    imu.init();
    Serial.println("OK");
    delay(200);

    Serial.print("[3/5] GPS... ");
    gps.init();
    Serial.println("OK");
    delay(200);
    
    Serial.print("[4/5] BLE... ");
    // If it hangs here, the BLE module is failing. 
    // Check BleManager.h -> it has a while(1) loop on failure!
    ble.begin(); 
    Serial.println("OK");
    delay(200);

    Serial.print("[5/5] Starting Threads... ");
    bleThread.start(bleTask);
    gpsThread.start(gpsTask);
    imuThread.start(imuTask);
    netThread.start(netTask);
    displayThread.start(displayTask);
    Serial.println("OK");

    // 4. HANDOFF
    systemReady = true;
    Serial.println("--- SYSTEM RUNNING (GREEN LED) ---");
    setStatusColor(false, true, false); // GREEN = Success
}

// ----------------------- LOOP ------------------------
void loop() {
    // Blink Green LED softly to show the Main Thread is alive
    digitalWrite(LEDG, LOW); 
    delay(500);

}