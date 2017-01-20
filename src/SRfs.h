//Класс для раьоты с файловой системой SPIFFS

#ifndef SRFS_H
#define SRFS_H

#include <FS.h>
#include <ESP8266WebServer.h>

class SRfs {
private:
  File fsUploadFile;
  ESP8266WebServer* HTTPhandle_h;

  String getContentType(String filename);

public:
  SRfs();
  void startFS(ESP8266WebServer& HTTPhandle);
  bool handleFileRead(String path);
  void handleFileUpload(void);
  void handleFileDelete(void);
  void handleFileCreate(void);
  void handleFileList(void);
 };

#endif
