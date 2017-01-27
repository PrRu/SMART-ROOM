#include "FS.h"
#include "SMART_ROOM.h"
#include "SRfs.h"
#include <ArduinoJson.h>
#include <ESP8266SSDP.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <aREST.h>

//////////////////////////////////////////////////////////////////////////////
//                           Глобальные объекты                             //
//////////////////////////////////////////////////////////////////////////////
WiFiServer SERVERaREST(8080); // aREST и сервер для него
aREST rest;                   // Определяем aREST
ESP8266WebServer HTTPserv; // Web интерфейс для устройства
SRfs WebfileSystem; // Web интерфес для файловой системы

//////////////////////////////////////////////////////////////////////////////
//                        Первоначальные настройки                          //
//////////////////////////////////////////////////////////////////////////////
void setup() {
  // Настраиваем вывод отладки
  Serial.begin(115200);
  delay(1000);
  Serial.println("-------------------------------------");
  Serial.println("---------SMART-ROOM START------------");
  //Загрузка файла конфигурации
  SPIFFS.begin();
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    while (true) {
      delay(1);
    };
  }
  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    while (true) {
      delay(1);
    };
  }
  std::unique_ptr<char[]> buf(
      new char[size]); //Буфер для чтения файла конфигурации
  configFile.readBytes(buf.get(), size);
  configFile.close();
  StaticJsonBuffer<200> jsonBuffer; //Буфер для JSON объекта
  JsonObject &root = jsonBuffer.parseObject(buf.get());
  JsonObject &network_json = root["network"];
  if (!network_json.success()) {
    Serial.println("Failed to parse config file. Section: 'Network'");
    while (true) {
      delay(1);
    };
  }
  //Включаем WiFiManager
  WiFiManager wifiManager;
  //Сетевые настройки
  IPAddress _ip, _gw, _sn;
  _ip.fromString((const char *)network_json["ip"]);
  _gw.fromString((const char *)network_json["gateway"]);
  _sn.fromString((const char *)network_json["mask"]);
  wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);
  //Если не удалось подключиться клиентом запускаем режим AP
  // доступ к настройкам по адресу http://192.168.4.1
  wifiManager.autoConnect();
  WiFi.hostname((const char *)network_json["hostname"]);
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
  HTTPserv = ESP8266WebServer(80); //Объявляем Web интерфейс для устройства
  WebfileSystem.startFS(HTTPserv); //Запускаем Web интерфейс для ФС
  // SSDP дескриптор
  HTTPserv.on("/description.xml", HTTP_GET,
              []() { SSDP.schema(HTTPserv.client()); });
  // HTTP страницы для работы с FFS
  // Просмотр папки с файлами
  HTTPserv.on("/list", HTTP_GET, []() { WebfileSystem.handleFileList(); });
  // загрузка редактора editor
  HTTPserv.on("/edit", HTTP_GET, []() {
    if (!WebfileSystem.handleFileRead("/edit.htm"))
      HTTPserv.send(404, "text/plain", "FileNotFound");
  });
  // Создание файла
  HTTPserv.on("/edit", HTTP_PUT, []() { WebfileSystem.handleFileCreate(); });
  // Удаление файла
  HTTPserv.on("/edit", HTTP_DELETE, []() { WebfileSystem.handleFileDelete(); });
  // загрузка файла в ФС
  HTTPserv.on("/edit", HTTP_POST,
              []() { HTTPserv.send(200, "text/plain", ""); },
              []() { WebfileSystem.handleFileUpload(); });
  //Ответы на запросы JSON объектов
  HTTPserv.on("/dynamicstat.json", HTTP_GET, []() { dynamicStatJSON(); });
  HTTPserv.on("/staticstat.json", HTTP_GET, []() { staticStatJSON(); });
  // Если путь не опреден, то считываем файл для WEB сервера, если файл не
  // найден,
  // то 404 ошибка
  HTTPserv.onNotFound([]() {
    if (!WebfileSystem.handleFileRead(HTTPserv.uri()))
      HTTPserv.send(404, "text/plain", "FileNotFound");
  });
  // Добавляем функцию Update для перезаписи прошивки
  update();
  // Запускаем HTTP сервер
  HTTPserv.begin();
  Serial.println("HTTP Ready!");

  //запускаем SSDP сервис
  SSDP.setSchemaURL("description.xml");
  SSDP.setHTTPPort(80);
  SSDP.setName(String(WiFi.hostname()));
  SSDP.setSerialNumber("0000705" + String(ESP.getChipId()));
  SSDP.setURL("/");
  SSDP.setModelName("SMART-ROOM");
  SSDP.setModelNumber("v0.10");
  SSDP.setManufacturer("R.k.S.");
  SSDP.setManufacturerURL("http://0705.ru");
  SSDP.setDeviceType("upnp:rootdevice");
  SSDP.begin();
  Serial.println("SSDP Ready!");

  // включаем aREST и сервер к нему
  // регистрируем в aRest функции
  //  rest.function((char *)"lam", lampControl);
  // Определяем имя name и ИД ID устройства aREST
  rest = aREST(); //Инициализация aREST объекта
  rest.set_id((char *)"1");
  rest.set_name((char *)"aRest");
  // Запускаем сервер
  SERVERaREST.begin();
  Serial.println("aREST Ready!");
}

void loop() {
  // put your main code here, to run repeatedly:
  HTTPserv.handleClient(); //Обрабротка запросов HTTP сервера
  delay(1);
  // Handle REST calls
  WiFiClient client = SERVERaREST.available();
  if (!client) {
    return;
  }
  while (!client.available()) {
    delay(1);
  }
  rest.handle(client);
}

//////////////////////////////////////////////////////////////////////////////
//                                Функции                                   //
//////////////////////////////////////////////////////////////////////////////

// Отправка JSON сообщений
// Сообщение о текущем состояние устройства
void dynamicStatJSON(void) {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  root["wifi_rssi"] = WiFi.RSSI();
  root["free_heap"] = ESP.getFreeHeap();
  String output;
  root.printTo(output);
  HTTPserv.send(200, "application/json", output);
}

// Сообщение о параметрах и настройках
void staticStatJSON(void) {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  root["ssid"] = WiFi.SSID();
  root["ip"] = WiFi.localIP().toString();
  root["gateway"] = WiFi.gatewayIP().toString();
  root["subnet"] = WiFi.subnetMask().toString();
  root["hostname"] = WiFi.hostname();
  String output;
  root.printTo(output);
  HTTPserv.send(200, "application/json", output);
}

//Обновление прошивки
void update(void) {
  HTTPserv.on(
      "/update", HTTP_POST,
      []() {
        HTTPserv.sendHeader("Connection", "close");
        HTTPserv.sendHeader("Access-Control-Allow-Origin", "*");
        HTTPserv.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        ESP.restart();
      },
      []() {
        HTTPUpload &upload = HTTPserv.upload();
        if (upload.status == UPLOAD_FILE_START) {
          Serial.setDebugOutput(true);
          WiFiUDP::stopAll();
          Serial.printf("Update: %s\n", upload.filename.c_str());
          uint32_t maxSketchSpace =
              (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
          if (!Update.begin(maxSketchSpace)) { // start with max available size
            Update.printError(Serial);
          }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
          if (Update.write(upload.buf, upload.currentSize) !=
              upload.currentSize) {
            Update.printError(Serial);
          }
        } else if (upload.status == UPLOAD_FILE_END) {
          if (Update.end(true)) { // true to set the size to the current
                                  // progress
            Serial.printf("Update Success: %u\nRebooting...\n",
                          upload.totalSize);
          } else {
            Update.printError(Serial);
          }
          Serial.setDebugOutput(false);
        }
        yield();
      });
}
