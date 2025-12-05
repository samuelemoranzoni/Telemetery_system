# Smart Bicycle Telemetry System - IoT Project

This repository contains the firmware for an advanced bicycle telemetry system powered by an **Arduino Portenta H7**. The system acts as a central "brain" that fuses data from multiple sensors (IMU, GPS, BLE Heart Rate Monitor) and transmits real-time metrics to the Cloud via MQTT.

## System Architecture
The project follows a modular Object-Oriented architecture with 5 main managers:
1. **BleManager:** Connects wirelessly to a Polar OH1 sensor.
2. **ImuManager:** Processes physics data from an MPU6050 (I2C).
3. **GpsManager:** Handles navigation data from an L76X GNSS module (UART).
4. **DisplayManager:** Visualizes data on a 1.5" OLED screen (SPI).
5. **NetworkManager:** Handles Wi-Fi connectivity and MQTT transmission.

---

## Telemetry Metrics Explained

The system computes and transmits the following metrics. Some are raw sensor data, others are computed on the Edge (on the Portenta).

### 1. Physiology (Athlete Performance)
- **Heart Rate (BPM)**
  - **Source:** Polar OH1 (via Bluetooth Low Energy).
  - **Description:** Real-time heart beats per minute.
  - **Use Case:** Monitoring athlete effort and training zones.

### 2. Bike Physics (Vehicle Dynamics)
- **G-Force (Total Acceleration)**
  - **Source:** MPU6050 Accelerometer.
  - **Computation:** `sqrt(ax^2 + ay^2 + az^2) / 9.81`.
  - **Description:** The total force acting on the bike (normalized to 1G gravity).
  - **Interpretation:** 1.0G = Cruising; >1.5G = Braking/Cornering; >3.0G = Impact/Crash.

- **Slope (Uphill/Downhill)**
  - **Source:** MPU6050 Accelerometer (Y-Axis vs Z-Axis).
  - **Computation:** `atan2(ay, az) * 57.29` (radians to degrees).
  - **Description:** The longitudinal inclination of the bike. Positive = Uphill, Negative = Downhill.

- **Lean Angle (Roll)**
  - **Source:** MPU6050 Accelerometer (X-Axis vs Z-Axis).
  - **Computation:** `atan2(ax, az) * 57.29`.
  - **Description:** The lateral tilt of the bike during turns. Essential for cornering analysis (MotoGP style).

- **Vibration Index (Road Quality)**
  - **Source:** MPU6050 Accelerometer (Z-Axis deviation).
  - **Computation:** `abs(az - 1G) * 100`.
  - **Description:** Quantifies road roughness.
  - **Interpretation:** 0-5 = Smooth Asphalt; 10-20 = Gravel/Cobblestone; >30 = Potholes.

- **Crash Detection**
  - **Source:** Computed Logic.
  - **Logic:** `IF (G-Force > 3.5G) THEN CRASH = TRUE`.
  - **Description:** Binary flag indicating a potential accident or severe impact.

### 3. Navigation (GPS Data)
- **Speed (Real)**
  - **Source:** L76X GNSS Module (Doppler shift).
  - **Description:** Accurate ground speed in km/h.

- **Altitude**
  - **Source:** L76X GNSS Module (Triangulation).
  - **Description:** Height above mean sea level in meters.

- **Odometer (Trip Distance)**
  - **Source:** Computed from GPS coordinates.
  - **Computation:** Cumulative sum of Haversine distance between consecutive points (1Hz).
  - **Description:** Total distance traveled in km since boot.

- **Average Speed**
  - **Source:** Computed.
  - **Computation:** `Total Distance / Elapsed Time`.
  - **Description:** Average pace of the session in km/h.

---

## ☁️ Connectivity (IoT)
- **Protocol:** MQTT over Wi-Fi.
- **Broker:** HiveMQ (Public).
- **Data Format:** JSON Payload (Frequency: 1Hz) containing all metrics for efficient logging and dashboard visualization.
