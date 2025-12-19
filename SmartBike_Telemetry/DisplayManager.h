#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H
#include <U8g2lib.h>

class DisplayManager {
private:
    U8G2_SSD1327_WS_128X128_F_4W_SW_SPI u8g2;
public:
    DisplayManager() : u8g2(U8G2_R0, 9, 8, 6, 7, 5) {}

    void init() {
        u8g2.begin();
        u8g2.enableUTF8Print();
        u8g2.setContrast(255);
    }

    void drawTelemetry(int bpm, double speed, double acceleration, float lateralG, double climbAngle,
                       bool wifiOk, bool bleOk, bool crash) {
        u8g2.clearBuffer();

        u8g2.setFont(u8g2_font_helvB08_tr);
        u8g2.drawStr(0, 10, "SMART BIKE");
        if(wifiOk) u8g2.drawStr(90, 10, "W");
        if(bleOk)  u8g2.drawStr(105, 10, "B");
        u8g2.drawHLine(0, 12, 128);

        u8g2.setFont(u8g2_font_ncenB12_tr);
        u8g2.setCursor(0, 30); u8g2.print("SPD: "); u8g2.print(speed, 1); u8g2.print(" km/h");
        u8g2.setCursor(0, 45); u8g2.print("HR: "); u8g2.print(bpm);
        u8g2.setCursor(0, 60); u8g2.print("ACC: "); u8g2.print(acceleration, 2);
        u8g2.setCursor(0, 75); u8g2.print("LATG: "); u8g2.print(lateralG, 2);
        u8g2.setCursor(0, 90); u8g2.print("CLIMB: "); u8g2.print(climbAngle, 1); u8g2.print("°");
        u8g2.setCursor(0, 105); u8g2.print("Crash: "); u8g2.print(crash ? "YES" : "NO");

        u8g2.drawFrame(0, 0, 128, 128);
        u8g2.sendBuffer();
    }
};

#endif
