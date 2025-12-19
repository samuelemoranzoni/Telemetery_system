#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <ArduinoBLE.h>

class BLEManager {
private:
    BLEDevice _peripheral;
    BLECharacteristic _hrChar;

    int _currentHeartRate = 0;

    bool connected = false;
    bool scanning  = false;

    unsigned long scanStart = 0;
    const unsigned long SCAN_TIMEOUT = 8000;   // 8 seconds
    const unsigned long SCAN_PAUSE   = 2000;   // pause before rescan

    unsigned long lastScanStop = 0;

public:
    void begin() {
        if (!BLE.begin()) {
            Serial.println("BLE init failed!");
            while (1);
        }

        Serial.println("BLE started");
        startScan();
    }

    void startScan() {
        Serial.println("BLE scanning...");
        BLE.scan();
        scanning = true;
        scanStart = millis();
    }

    void stopScan() {
        BLE.stopScan();
        scanning = false;
        lastScanStop = millis();
    }

    void update() {
        BLE.poll();

        // -------------------------
        // SCANNING STATE
        // -------------------------
        if (!connected && scanning) {
            BLEDevice dev = BLE.available();

            if (dev && dev.localName().indexOf("Polar") >= 0) {
                Serial.println("Found Polar! Connecting...");
                stopScan();

                if (dev.connect() && dev.discoverAttributes()) {
                    _hrChar = dev.characteristic("2a37");
                    if (_hrChar && _hrChar.canSubscribe()) {
                        _hrChar.subscribe();
                        _peripheral = dev;
                        connected = true;
                        Serial.println("BLE connected");
                        return;
                    }
                }

                // Failed connection → restart scan
                startScan();
                return;
            }

            // Scan timeout → pause → rescan
            if (millis() - scanStart > SCAN_TIMEOUT) {
                Serial.println("BLE scan timeout");
                stopScan();
            }
            return;
        }

        // -------------------------
        // RESCAN AFTER PAUSE
        // -------------------------
        if (!connected && !scanning) {
            if (millis() - lastScanStop > SCAN_PAUSE) {
                startScan();
            }
            return;
        }

        // -------------------------
        // CONNECTED STATE
        // -------------------------
        if (connected) {
            if (!_peripheral.connected()) {
                Serial.println("BLE disconnected");
                connected = false;
                _currentHeartRate = 0;
                stopScan();
                return;
            }

            if (_hrChar.valueUpdated()) {
                const uint8_t* b = _hrChar.value();
                _currentHeartRate =
                    (b[0] & 1) ? (b[1] | (b[2] << 8)) : b[1];
            }
        }
    }

    int getHeartRate() { return _currentHeartRate; }
    bool isDeviceConnected() { return connected; }

    // WiFi can safely start if BLE is either connected or idle
    bool isReady() { return connected || !scanning; }
};

#endif
