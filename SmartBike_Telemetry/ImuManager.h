#ifndef IMU_MANAGER_H
#define IMU_MANAGER_H

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

class ImuManager {
private:
    Adafruit_MPU6050 mpu;

    // Core metrics
    float gForce = 1.0;         // Total G magnitude
    float slope = 0.0;          // Pitch (up/down) in degrees
    float leanAngle = 0.0;      // Roll (left/right) in degrees
    float lateralG = 0.0;       // Turn G-force
    float vibration = 0.0;      // Road vibration
    bool crashDetected = false;
    float tempC = 0.0;

public:
    void init() {
        if (!mpu.begin()) {
            Serial.println("IMU Error");
            return;
        }

        mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

        Serial.println("IMU OK");
    }

    void update() {
        sensors_event_t a, g, temp;
        mpu.getEvent(&a, &g, &temp);

        // -----------------------------
        // 1️⃣ TOTAL G-FORCE (magnitude)
        // -----------------------------
        float ax = a.acceleration.x;
        float ay = a.acceleration.y;
        float az = a.acceleration.z;

        float totalAcc = sqrt(ax*ax + ay*ay + az*az);
        gForce = totalAcc / 9.81;

        // -----------------------------
        // 2️⃣ NORMALIZE GRAVITY VECTOR
        // -----------------------------
        float norm = sqrt(ax*ax + ay*ay + az*az);
        if (norm == 0) return;

        float gx = ax / norm;
        float gy = ay / norm;
        float gz = az / norm;

        // -----------------------------
        // 3️⃣ SLOPE (PITCH) & LEAN (ROLL)
        // -----------------------------
        // Bicycle reference:
        // X = left/right
        // Y = forward/back
        // Z = up/down

        leanAngle = atan2(gx, gz) * 57.2958;   // Roll
        slope     = atan2(gy, gz) * 57.2958;   // Pitch

        // -----------------------------
        // 4️⃣ REMOVE GRAVITY FROM XY
        // -----------------------------
        float linAx = ax - gx * 9.81;
        float linAy = ay - gy * 9.81;

        // -----------------------------
        // 5️⃣ LATERAL G (TURN FORCE)
        // -----------------------------
        lateralG = sqrt(linAx*linAx + linAy*linAy) / 9.81;

        // Kill noise when stopped
        if (lateralG < 0.02) lateralG = 0.0;

        // -----------------------------
        // 6️⃣ VIBRATION (ROAD QUALITY)
        // -----------------------------
        float verticalG = abs(az / 9.81);
        vibration = abs(verticalG - 1.0) * 100.0;

        // -----------------------------
        // 7️⃣ CRASH DETECTION
        // -----------------------------
        crashDetected = (gForce > 3.5);

        // -----------------------------
        // 8️⃣ TEMPERATURE
        // -----------------------------
        tempC = temp.temperature;
    }

    // -----------------------------
    // GETTERS
    // -----------------------------
    float getGForce()     { return gForce; }
    float getSlope()      { return slope; }
    float getLean()       { return leanAngle; }
    float getLateralG()   { return lateralG; }
    float getVibration()  { return vibration; }
    bool  isCrash()       { return crashDetected; }
    float getTemp()       { return tempC; }
};

#endif
