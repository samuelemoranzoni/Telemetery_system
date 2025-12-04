# Bike Telemetry — Lean, Vibration & Crash Detection

This repository contains an Arduino sketch for basic bike telemetry using an IMU (e.g. MPU6050).
It computes three main derived metrics in real-time:

## 1. Lean Angle (Roll)
- Computed from accelerometer readings using `atan2(ax, az)` to obtain roll in degrees.
- Optionally fused with gyroscope readings via a complementary filter to reduce drift and improve responsiveness.
- Use case: cornering technique analysis, safety alerts (e.g. fall detection).
- Typical threshold example: `|lean| > 45°` can be used as a fall-warning condition (tune experimentally).

## 2. Vibration Index (Road Roughness)
- Based on the Z-axis accelerometer signal (vertical).
- We compute the **standard deviation** of Z (in g) over a sliding window to quantify surface roughness.
- The index provided is `stddev * 100` for human-readable scale; calibrate scaling for your mount and sample rate.
- Suggested interpretation (example; calibrate empirically):
  - 0–10: smooth asphalt
  - 10–30: gravel / cobblestone
  - >30: very rough / dangerous pothole

## 3. Crash Detection
- Compute total acceleration magnitude `sqrt(ax^2 + ay^2 + az^2)` and convert to `g` units.
- If `gForce > 3.5` → `crashDetected = true`. This is a simple peak threshold; improve by adding time-windowed peaks or combining with tilt/orientation changes.
- Consider adding additional logic: sudden large change in lean + high g, or a period of inactivity after high g to indicate rider incapacitation.
