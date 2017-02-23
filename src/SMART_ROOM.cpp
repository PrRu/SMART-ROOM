#include "SMART_ROOM.h"

//////////////////////////////////////////////////////////////////////////////
//                           Глобальные объекты                             //
//////////////////////////////////////////////////////////////////////////////
TwoWire i2c_conn;
WiFiClient wfclient; //подключение mqtt к wifi
PubSubClient mqtt_client(wfclient);
bool shouldSaveConfig = false; //флаг требуется сохранить конфигурацию
display disp_i2c(&i2c_conn); //OLED дисплей на шине I2C
io_cntr io_block(&i2c_conn); //Блоко входов / выходов на PCF8574
unsigned long tickTack = 0;

//////////////////////////////////////////////////////////////////////////////
//                        Первоначальные настройки                          //
//////////////////////////////////////////////////////////////////////////////
void setup() {
  // Настраиваем вывод отладки
  Serial.begin(115200);
  delay(1000);
  Serial.println("-------------------------------------");
  Serial.println("---------SMART-ROOM START------------");
  //Настройка входов/выходов
  pinMode(goSetting, INPUT); //При "0" принудительный переход к настройкам
  //Настройка интерфейса i2c
  i2c_conn.begin(sda_pin, scl_pin);
  i2c_conn.setClock(100000L);
  //Настройка входов / выходов
  io_block.begin();
  Serial.println("IO block started");
  //Настройка дисплея
  disp_i2c.begin();
  Serial.println("OLED display started");
  //Загрузка файла конфигурации
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      // file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        configFile.close();
        DynamicJsonBuffer jsonBuffer;
        JsonObject &json = jsonBuffer.parseObject(buf.get());
        if (json.success()) {
          Serial.println("\nparsed json:");
          json.prettyPrintTo(Serial);
          Serial.println("\n-------------------------------------");
          if ((const char *)json["mqtt_server"])
            strcpy(mqtt_server, json["mqtt_server"]);
          if ((const char *)json["mqtt_port"])
            strcpy(mqtt_portStr, json["mqtt_port"]);
          if ((const char *)json["ip"])
            strcpy(ip_str, json["ip"]);
          if ((const char *)json["gateway"])
            strcpy(gateway_str, json["gateway"]);
          if ((const char *)json["subnet"])
            strcpy(mask_str, json["subnet"]);
          if ((const char *)json["hostname"])
            strcpy(hostname_str, json["hostname"]);
        } else {
          Serial.println("failed to load json config!");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS!");
  }

  //Сетевые настройки
  IPAddress _ip, _gw, _sn;
  _ip.fromString(ip_str);
  _gw.fromString(gateway_str);
  _sn.fromString(mask_str);
  //Пытаемся подключится по WiFi, если не получается или нажата кнопка
  //вызова порта конфигурации, то запускаем конфигуратор
  boolean startConfigPortal = false;
  if (digitalRead(goSetting) == LOW) {
    startConfigPortal = true;
  }
  WiFi.mode(WIFI_STA);
  if (WiFi.SSID()) {
    Serial.println("Using saved credentials");
    ETS_UART_INTR_DISABLE();
    wifi_station_disconnect();
    ETS_UART_INTR_ENABLE();
    WiFi.begin();
    WiFi.config(_ip, _gw, _sn);
  } else {
    Serial.println("No saved credentials");
    startConfigPortal = true;
  }

  WiFi.waitForConnectResult();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Failed to connect Wifi");
    startConfigPortal = true;
  }
  //Вызов портала конфигурации
  if (startConfigPortal) {
    //Выводим надпись о запуске портала конфигурации
    disp_i2c.cnfgPortalstr();
    //Включаем WiFiManager
    WiFiManager wifiManager;
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);
    WiFiManagerParameter custom_hostname("hostname", "Hostname", hostname_str,
                                         40);
    WiFiManagerParameter custom_mqtt_server("server", "MQTT server",
                                            mqtt_server, 20);
    WiFiManagerParameter custom_mqtt_port("port", "MQTT port", mqtt_portStr, 6);
    wifiManager.addParameter(&custom_hostname);
    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.addParameter(&custom_mqtt_port);

    wifiManager.startConfigPortal("SMART-ROOM_Config");
    if (shouldSaveConfig) {
      //Сначала читаем обновленные параметры
      strcpy(mqtt_server, custom_mqtt_server.getValue());
      strcpy(mqtt_portStr, custom_mqtt_port.getValue());
      strcpy(hostname_str, custom_hostname.getValue());
      Serial.println("saving config");
      DynamicJsonBuffer jsonBuffer;
      JsonObject &json = jsonBuffer.createObject();
      json["ip"] = WiFi.localIP().toString();
      json["gateway"] = WiFi.gatewayIP().toString();
      json["subnet"] = WiFi.subnetMask().toString();
      json["hostname"] = hostname_str;
      json["mqtt_server"] = mqtt_server;
      json["mqtt_port"] = mqtt_portStr;
      //Печатаем в терминал получившиеся настройки
      json.prettyPrintTo(Serial);
      File configFile = SPIFFS.open("/config.json", "w");
      if (!configFile) {
        Serial.println("failed to open config file for writing");
      } else {
        json.printTo(configFile);
        configFile.close();
      }
      shouldSaveConfig = false;
    }
  }

  //установка hostname
  WiFi.hostname(hostname_str);
  //если подключение к точке доступа произошло сообщаем
  Serial.println("WiFi connected");
  delay(1000);
  //вывод текущих параметров WiFi подключения
  Serial.println("--------------------------------------");
  Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
  Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
  Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("Gateway IP: %s\n", WiFi.gatewayIP().toString().c_str());
  Serial.printf("Subnet mask: %s\n", WiFi.subnetMask().toString().c_str());
  Serial.printf("Hostname: %s\n", WiFi.hostname().c_str());
  Serial.println("--------------------------------------");
  //Настройка сетевых возможностей
  //настройка MQTT
  IPAddress mqtt_server_ip;
  mqtt_server_ip.fromString(mqtt_server);
  mqtt_client.set_server(mqtt_server_ip, atoi(mqtt_portStr));
}

void loop() {
  //Работа с MQTT
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqtt_client.connected()) {
      if (mqtt_client.connect(String(WiFi.hostname()))) {
        mqtt_client.set_callback(callback_subscr);
        mqtt_client.subscribe(
            String(WiFi.hostname()) +
            "/#"); //подписываемся  на все темы для этого устройства
        Serial.println("MQTT Ready!");
      } else {
        Serial.println("Could not connect to MQTT server!");
        delay(5000);
      }
    }
    if (mqtt_client.connected())
      mqtt_client.loop();
  }
  //обновление дисплея
  disp_i2c.update();
  //обновление таймера для переодических событий
  timerUpdate();
}

//////////////////////////////////////////////////////////////////////////////
//                                Функции                                   //
//////////////////////////////////////////////////////////////////////////////

// Функция получения данных от MQTT брокера
void callback_subscr(const MQTT::Publish &pub) {
  String topic = pub.topic();
  String value = pub.payload_string();
  Serial.println("Topic: " + topic);
  Serial.println("Payload: " + value);
}

// Обратный вызов для оповещения об необходимости сохранить конфигурацию
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

// Таймер для переодических событий
void timerUpdate(void) {
  unsigned long currTick = millis();
  //Переодическая рассылка информации о состояние устройства
  if (currTick > (tickTack + info_period)) {
    sndPeriodicInfo();
    tickTack = currTick;
  };
}

//Переодическая рассылка по MQTT информации о состоянии устройства
void sndPeriodicInfo(void){
  //Если есть подключение mqtt, то публикуем топик
  if (mqtt_client.connected()) {
    //Формируем JSON строку для отправки по MQTT
    DynamicJsonBuffer jsonBuffer;
    JsonObject &json = jsonBuffer.createObject();
    json["wifi_ssid"] = WiFi.SSID();
    json["wifi_rssi"] = WiFi.RSSI();
    json["ip"] = WiFi.localIP().toString();
    String Payload;
    json.printTo(Payload);
    String Topic = "info/" + WiFi.hostname();
    mqtt_client.publish(Topic, Payload);
  }

}
