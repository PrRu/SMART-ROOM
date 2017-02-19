#include "display.h"

display::display(void) {}

//Инициализация дисплея
void display::begin(void) {
  oled_disp.begin();
  oled_disp.print("SMART-ROOM", 3, 3);
  oled_disp.print("v 1.0", 5, 5);
}

//Обновление информации на дисплее
void display::update(void) {
  unsigned long currTick = millis();
  if (currTick > (lastTick + updInterval)) {
    switch (currScreen) {
    case 0:
      oled_disp.clear();
      currScreen = 1;
    case 1:
      showScreen_0();
      currScreen = 2;
      break;
    case 2:
      showScreen_1();
      currScreen = 1;
      break;
    default:
      currScreen = 0;
    }
    lastTick = currTick;
  }
}

//Показ первого экрана
void display::showScreen_0(void) {
  if (WiFi.status() == WL_CONNECTED) {
    char buf[17];
    sprintf(buf, "%-16s", WiFi.SSID().c_str());
    oled_disp.print(buf, 0);
    sprintf(buf, "%-16s", WiFi.hostname().c_str());
    oled_disp.print(buf, 2);
  } else {
    oled_disp.clear();
    oled_disp.print("WiFi", 0);
    oled_disp.print("not connected!", 2);
  }
}

//Показ второго экрана
void display::showScreen_1(void) {
  if (WiFi.status() == WL_CONNECTED) {
    char buf[17];
    sprintf(buf, "%12d dBm", WiFi.RSSI());
    oled_disp.print(buf, 0);
    sprintf(buf, "%-16s", WiFi.localIP().toString().c_str());
    oled_disp.print(buf, 2);

    if (WiFi.RSSI() <= -100) sprintf(buf, "     | ");
    if ((WiFi.RSSI() >= -101) && (WiFi.RSSI() <= -91)) sprintf(buf, "    *| ");
    if ((WiFi.RSSI() >= -90) && (WiFi.RSSI() <= -81)) sprintf(buf, "   **| ");
    if ((WiFi.RSSI() >= -80) && (WiFi.RSSI() <= -71)) sprintf(buf, "  ***| ");
    if ((WiFi.RSSI() >= -70) && (WiFi.RSSI() <= -61)) sprintf(buf, " ****| ");
    if (WiFi.RSSI() >= -60) sprintf(buf, "*****| ");

    oled_disp.print(buf, 0);

  } else {
    oled_disp.clear();
    oled_disp.print("WiFi", 0);
    oled_disp.print("not connected!", 2);
  }
}
