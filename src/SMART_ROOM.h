#ifndef SMART_ROOM_H
#define SMART_ROOM_H

#include <PubSubClient.h>
#include "FS.h"
#include <ArduinoJson.h>
#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <Wire.h>
#include "Display.h"

//Настройки
#define goSetting 16
#define scl_pin 5
#define sda_pin 4
//Настройки по умолчанию
char ip_str[20] = "192.168.2.100\0";
char gateway_str[20] = "192.168.2.1\0";
char mask_str[20] = "255.255.255.0\0";
char hostname_str[40] = "S-ROOM\0";
char mqtt_server[20] = "192.168.2.1\0";
char mqtt_portStr[11] = "1883\0";

void callback_subscr(const MQTT::Publish &pub);
void saveConfigCallback (void);


#endif
