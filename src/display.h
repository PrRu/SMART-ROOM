#ifndef DISPLAY_H
#define DISPLAY_H

#include <OLED.h>
#include <ESP8266WiFi.h>

#define updInterval 5000

class display {

public:
  display(void);
  void begin(void); //Инициализация дисплея
  void update(void); //Обновление информации на дисплее

private:
  OLED oled_disp;
  unsigned long lastTick = 0;
  uint8_t currScreen = 0;

  void showScreen_0(void);
  void showScreen_1(void);

};

#endif
