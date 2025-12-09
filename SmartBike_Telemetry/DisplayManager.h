#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H
#include <U8g2lib.h>

class DisplayManager {
  private:
    U8G2_SSD1327_WS_128X128_F_4W_SW_SPI u8g2;
  public:
    DisplayManager() : u8g2(U8G2_R0, 9, 8, 6, 7, 5) {}
    void init() { u8g2.begin(); u8g2.enableUTF8Print(); u8g2.setContrast(255); }

    void drawTelemetry(int bpm, float gf, double speed, int sats, bool wifiOk, bool bleOk) {
      u8g2.clearBuffer();

      // Header
      u8g2.setFont(u8g2_font_helvB08_tr);
      u8g2.drawStr(2, 10, "SMART BIKE");
      u8g2.drawHLine(0, 12, 128);

      // Icon
      if(wifiOk) u8g2.drawStr(90, 10, "W");
      if(bleOk)  u8g2.drawStr(105, 10, "B");

      // speed displayed
      u8g2.setFont(u8g2_font_ncenB18_tr);
      u8g2.setCursor(5, 50);
      u8g2.print(speed, 1);
      u8g2.setFont(u8g2_font_helvB08_tr); u8g2.print(" km/h");

      // Heart rate (bpm) displayed
      u8g2.setCursor(5, 75); 
      u8g2.print("HR: "); (bpm>0)? u8g2.print(bpm) : u8g2.print("--");

      // g-force displayed
      u8g2.setCursor(5, 90); 
      u8g2.print("G: "); u8g2.print(gf, 2);

      // number of satellites displayed
      u8g2.setCursor(5, 115);
      u8g2.print("Sats: "); u8g2.print(sats);
      if(sats < 4) u8g2.print(" (No Fix)");

      u8g2.drawFrame(0, 0, 128, 128);
      u8g2.sendBuffer();
    }
};
#endif