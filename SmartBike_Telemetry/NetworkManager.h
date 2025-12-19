#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H
#include <WiFi.h>
#include <ArduinoMqttClient.h>

class NetworkManager {
private:
    char ssid[50], pass[50];
    const char* broker = "broker.hivemq.com"; 
    int port = 1883; 
    String baseTopic = "portenta/bike/"; 
    WiFiClient wifiClient;
    MqttClient mqttClient;

    unsigned long wifiStartTime = 0;
    const unsigned long WIFI_TIMEOUT = 10000; // 10 sec
    bool connectingWiFi = false;

public:
    NetworkManager(const char* wifiSsid, const char* wifiPass) : mqttClient(wifiClient) {
        strcpy(ssid, wifiSsid);
        strcpy(pass, wifiPass);
    }

    void init() {
        if(WiFi.status() != WL_CONNECTED && !connectingWiFi) {
            Serial.print("Connecting WiFi: "); Serial.println(ssid);
            WiFi.begin(ssid, pass);
            wifiStartTime = millis();
            connectingWiFi = true;
        }
    }

    void update() {
        // Non-blocking WiFi check
        if(connectingWiFi) {
            if(WiFi.status() == WL_CONNECTED) {
                Serial.println("WiFi OK!");
                connectingWiFi = false;
                connectMqtt();
            } else if(millis() - wifiStartTime > WIFI_TIMEOUT) {
                Serial.println("WiFi connection timeout");
                WiFi.disconnect();
                connectingWiFi = false;
            }
        }

        // MQTT reconnect
        if(WiFi.status() == WL_CONNECTED) {
            mqttClient.poll();
            static unsigned long lastReconnect = 0;
            if(!mqttClient.connected() && millis() - lastReconnect > 5000) {
                connectMqtt();
                lastReconnect = millis();
            }
        }
    }

    void connectMqtt() {
        if(!mqttClient.connected()) {
            Serial.print("Connecting MQTT...");
            if(!mqttClient.connect(broker, port)) Serial.println(" MQTT Error");
            else Serial.println(" MQTT Connected!");
        }
    }

    bool isWiFiConnected() { return WiFi.status() == WL_CONNECTED; }
    bool isMqttConnected() { return mqttClient.connected(); }

    void sendTelemetry(int bpm, float gf, float slope, float lean, float vib, bool crash, 
                       float temp, double lat, double lon, double alt, double spd, double odo, double avg,
                       double acceleration, double climbAngle, float lateralG, float effort) {
        if(!isWiFiConnected()) return;
        if(!mqttClient.connected()) connectMqtt();

        // --- Send single topics ---
        mqttClient.beginMessage(baseTopic+"bpm"); mqttClient.print(bpm); mqttClient.endMessage();
        mqttClient.beginMessage(baseTopic+"gf");  mqttClient.print(gf);  mqttClient.endMessage();
        mqttClient.beginMessage(baseTopic+"slp"); mqttClient.print(slope); mqttClient.endMessage();
        mqttClient.beginMessage(baseTopic+"lean"); mqttClient.print(lean); mqttClient.endMessage();
        mqttClient.beginMessage(baseTopic+"vib"); mqttClient.print(vib); mqttClient.endMessage();
        mqttClient.beginMessage(baseTopic+"crash"); mqttClient.print(crash); mqttClient.endMessage();
        mqttClient.beginMessage(baseTopic+"temp"); mqttClient.print(temp); mqttClient.endMessage();
        mqttClient.beginMessage(baseTopic+"lat"); mqttClient.print(lat,6); mqttClient.endMessage();
        mqttClient.beginMessage(baseTopic+"lon"); mqttClient.print(lon,6); mqttClient.endMessage();
        mqttClient.beginMessage(baseTopic+"alt"); mqttClient.print(alt); mqttClient.endMessage();
        mqttClient.beginMessage(baseTopic+"spd"); mqttClient.print(spd); mqttClient.endMessage();
        mqttClient.beginMessage(baseTopic+"odo"); mqttClient.print(odo); mqttClient.endMessage();
        mqttClient.beginMessage(baseTopic+"avg"); mqttClient.print(avg); mqttClient.endMessage();

        // --- New metrics ---
        mqttClient.beginMessage(baseTopic+"acc"); mqttClient.print(acceleration,2); mqttClient.endMessage();
        mqttClient.beginMessage(baseTopic+"climb"); mqttClient.print(climbAngle,2); mqttClient.endMessage();
        mqttClient.beginMessage(baseTopic+"latg"); mqttClient.print(lateralG,2); mqttClient.endMessage();
        mqttClient.beginMessage(baseTopic+"effort"); mqttClient.print(effort,2); mqttClient.endMessage();

        // --- Send full JSON ---
        String json = "{";
        json += "\"bpm\":" + String(bpm) + ",";
        json += "\"gf\":" + String(gf) + ",";
        json += "\"slp\":" + String(slope) + ",";
        json += "\"lean\":" + String(lean) + ",";
        json += "\"vib\":" + String(vib) + ",";
        json += "\"crash\":" + String(crash) + ",";
        json += "\"tmp\":" + String(temp) + ",";
        json += "\"lat\":" + String(lat,6) + ",";
        json += "\"lon\":" + String(lon,6) + ",";
        json += "\"alt\":" + String(alt) + ",";
        json += "\"spd\":" + String(spd) + ",";
        json += "\"odo\":" + String(odo) + ",";
        json += "\"avg\":" + String(avg) + ",";
        json += "\"acc\":" + String(acceleration,2) + ",";
        json += "\"climb\":" + String(climbAngle,2) + ",";
        json += "\"latg\":" + String(lateralG,2) + ",";
        json += "\"effort\":" + String(effort,2);
        json += "}";

        mqttClient.beginMessage(baseTopic+"json"); 
        mqttClient.print(json); 
        mqttClient.endMessage();

        Serial.println(">> SENT: " + json);
    }
};

#endif
