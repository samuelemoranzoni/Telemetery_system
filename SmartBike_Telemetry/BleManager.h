#include <ArduinoBLE.h>

class BLEManager {
private:
    BLEDevice _peripheral;
    BLECharacteristic _hrChar;
    int _currentHeartRate = 0;
    bool _isConnected = false;

public:
    // Initialize BLE
    void begin() {
        if (!BLE.begin()) {
            Serial.println("Starting BLE failed!");
            while (1);
        }
        Serial.println("BLE Manager started. Scanning...");
        BLE.scan();
    }

    // This checks for connections and reads data
    void update() {
        // 1. Keep BLE radio active
        BLE.poll();

        // CASE A: We are NOT connected. Look for a device.
        if (!_isConnected) {
            BLEDevice foundDevice = BLE.available();

            if (foundDevice) {
                if (foundDevice.localName().indexOf("Polar") >= 0) {

                    BLE.stopScan();
                    Serial.println("Found Polar! Connecting...");

                    if (foundDevice.connect()) {
                        Serial.println("Connected!");
                        _peripheral = foundDevice; // Save the device

                        // Discover attributes
                        if (_peripheral.discoverAttributes()) {
                            _hrChar = _peripheral.characteristic("2a37");

                            if (_hrChar && _hrChar.canSubscribe()) {
                                _hrChar.subscribe();
                                _isConnected = true; // Mark as connected
                            }
                            else {
                                Serial.println("Characteristic error.");
                                _peripheral.disconnect();
                                BLE.scan();
                            }
                        }
                        else {
                            Serial.println("Attribute discovery failed.");
                            _peripheral.disconnect();
                            BLE.scan();
                        }
                    }
                    else {
                        Serial.println("Connection failed. Scanning...");
                        BLE.scan();
                    }
                }
            }
        }

        // CASE B: We ARE connected. Read the data.
        else {
            // Check if device is still actually connected
            if (!_peripheral.connected()) {
                Serial.println("Device disconnected! Scanning again...");
                _isConnected = false;
                _currentHeartRate = 0;
                BLE.scan();
                return;
            }

            // Read the value if it updated
            if (_hrChar.valueUpdated()) {
                const uint8_t* bytes = _hrChar.value();

                // Your parsing logic
                _currentHeartRate = (bytes[0] & 1) ? (bytes[1] | (bytes[2] << 8)) : bytes[1];

                // Debug print inside the manager
                Serial.print("BPM: ");
                Serial.println(_currentHeartRate);
            }
        }
    }

    // Helper to get the value in your main loop if you need it later
    int getHeartRate() {
        return _currentHeartRate;
    }

    bool isDeviceConnected() {
        return _isConnected;
    }
};