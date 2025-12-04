#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <WiFi.h>
#include <ArduinoMqttClient.h>

class NetworkManager {
  private:
    char ssid[50];
    char pass[50];
    const char* broker = "broker.hivemq.com"; 
    int port = 1883;
    String baseTopic = "portenta/bike/"; 
    WiFiClient wifiClient;
    MqttClient mqttClient;

  public:
    NetworkManager(const char* wifiSsid, const char* wifiPass) : mqttClient(wifiClient) {
      strcpy(ssid, wifiSsid);
      strcpy(pass, wifiPass);
    }

    void init() {
      WiFi.begin(ssid, pass);
      int t=0; while(WiFi.status()!=WL_CONNECTED && t<20){ delay(500); Serial.print("."); t++; }
      if(WiFi.status()==WL_CONNECTED) connectMqtt();
    }

    void connectMqtt() { mqttClient.connect(broker, port); }
    void update() { if(WiFi.status()==WL_CONNECTED) mqttClient.poll(); }

    // FUNZIONE DI INVIO COMPLETA
    void sendTelemetry(int bpm, float gf, float slope, float lean, float vib, bool crash, 
                       float temp, double lat, double lon, double alt, double spd, double odo, double avg) {
      
      if (WiFi.status() == WL_CONNECTED) {
        if (!mqttClient.connected()) connectMqtt();

        // 1. Invio metriche veloci singole (per i gauge dell'App)
        
        mqttClient.beginMessage(baseTopic + "bpm"); mqttClient.print(bpm); mqttClient.endMessage();
        mqttClient.beginMessage(baseTopic + "gf");  mqttClient.print(gf);  mqttClient.endMessage();
        mqttClient.beginMessage(baseTopic + "slp"); mqttClient.print(slope); mqttClient.endMessage();
        mqttClient.beginMessage(baseTopic + "lean"); mqttClient.print(lean); mqttClient.endMessage();
        mqttClient.beginMessage(baseTopic + "crash"); mqttClient.print(crash); mqttClient.endMessage();
        mqttClient.beginMessage(baseTopic + "vib"); mqttClient.print(vib); mqttClient.endMessage();
        mqttClient.beginMessage(baseTopic + "temp"); mqttClient.print(temp); mqttClient.endMessage();
        mqttClient.beginMessage(baseTopic + "lat"); mqttClient.print(lat, 6); mqttClient.endMessage();
        mqttClient.beginMessage(baseTopic + "lon"); mqttClient.print(lon, 6); mqttClient.endMessage();
        mqttClient.beginMessage(baseTopic + "alt"); mqttClient.print(alt); mqttClient.endMessage();
        mqttClient.beginMessage(baseTopic + "spd"); mqttClient.print(spd); mqttClient.endMessage();
        mqttClient.beginMessage(baseTopic + "odo"); mqttClient.print(odo); mqttClient.endMessage();
        mqttClient.beginMessage(baseTopic + "avg"); mqttClient.print(avg); mqttClient.endMessage();


        
        
        
        // 2. Invio JSON COMPLETO (Database Log)
        String json = "{";
        // Fisiologia & Fisica
        json += "\"bpm\":" + String(bpm) + ",";
        json += "\"gf\":" + String(gf) + ",";
        json += "\"slp\":" + String(slope) + ",";
        json += "\"lean\":" + String(lean) + ",";
        json += "\"vib\":" + String(vib) + ",";
        json += "\"crash\":" + String(crash) + ",";
        json += "\"tmp\":" + String(temp) + ",";
        // Navigazione GPS
        json += "\"lat\":" + String(lat, 6) + ",";
        json += "\"lon\":" + String(lon, 6) + ",";
        json += "\"alt\":" + String(alt) + ",";
        json += "\"spd\":" + String(spd) + ",";
        json += "\"odo\":" + String(odo) + ",";
        json += "\"avg\":" + String(avg);
        json += "}";

        mqttClient.beginMessage(baseTopic + "json");
        mqttClient.print(json);
        mqttClient.endMessage();
        
        Serial.println(">> SENT: " + json);
      }
    }
};
#endif