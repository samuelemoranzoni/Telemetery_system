#ifndef IMU_MANAGER_H
#define IMU_MANAGER_H

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

class ImuManager {
  private:
    Adafruit_MPU6050 mpu;   // The sensor object
    float gForce = 0.0;     // Total force (gravity + movement)
    float slope = 0.0;      // Uphill/Downhill angle
    float leanAngle = 0.0;  // Left/Right tilt in turns
    float vibration = 0.0;  // How much the road shakes
    bool crashDetected = false; // Did we crash?
    float tempC = 0.0;      // Ambient temperature

  public:
    // Setup function: runs once at startup
    void init() {
      if (!mpu.begin()) { 
        Serial.println("IMU Error"); // Error if sensor is disconnected
        return; 
      }
      // Set sensor range to 8G (so it doesn't max out on bumps)
      mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
      // Filter out small vibrations (21Hz bandwidth)
      mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    }

    // Main logic: runs constantly in the loop
    void update() {
      sensors_event_t a, g, temp;
      mpu.getEvent(&a, &g, &temp); // Read new data from hardware

      // 1. CALCULATE G-FORCE (Total Magnitude)
      // We use Pythagoras theorem in 3D: sqrt(x^2 + y^2 + z^2)
      // Divided by 9.81 to convert meters/sec^2 to "G" units
      float rawG = sqrt(sq(a.acceleration.x) + sq(a.acceleration.y) + sq(a.acceleration.z));
      gForce = rawG / 9.81;

      // 2. CALCULATE SLOPE (Pitch / Y-Axis)
      // Trigonometry: angle between forward axis (Y) and gravity (Z)
      // * 57.29 converts Radians to Degrees
      slope = atan2(a.acceleration.y, a.acceleration.z) * 57.29; 

      // 3. CALCULATE LEAN ANGLE (Roll / X-Axis)
      // Same logic but using the sideways axis (X)
      leanAngle = atan2(a.acceleration.x, a.acceleration.z) * 57.29;

      // 4. CALCULATE VIBRATION (Road Quality)
      // We check how much the vertical axis (Z) deviates from normal gravity (1G).
      // Multiplied by 100 to make the number easier to read (e.g., 5, 10, 20)
      float zG = abs(a.acceleration.z / 9.81);
      vibration = abs(zG - 1.0) * 100; 

      // 5. CRASH DETECTION LOGIC
      // Simple threshold: if force is huge (> 3.5G), assume a crash
      if (gForce > 3.5) crashDetected = true; 
      else crashDetected = false;

      // Read internal temperature sensor
      tempC = temp.temperature;
    }

    // Getter functions 
    float getGForce() { return gForce; }
    float getSlope() { return slope; }
    float getLean() { return leanAngle; }
    float getVibration() { return vibration; }
    bool isCrash() { return crashDetected; }
    float getTemp() { return tempC; }
};
#endif