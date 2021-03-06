#ifndef _DISPLAY_H
#define _DISPLAY_H

#include <OLED.h>
#include <ESP8266WiFi.h>

#define updInterval 5000

class display {

public:
  display(TwoWire *UseWire);
  void begin(void); //Инициализация дисплея
  void update(void); //Обновление информации на дисплее
  void cnfgPortalstr(void); //Показ ин-ции о запуски WiFiManager

private:
  OLED* oled_disp;
  unsigned long lastTick = 0;
  uint8_t currScreen = 0;

  void showScreen_0(void);
  void showScreen_1(void);

};

#endif
