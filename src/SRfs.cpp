#include "SRfs.h"

//Конструктор
SRfs::SRfs(){};


void SRfs::startFS(ESP8266WebServer &HTTPhandle) {
  //Указатель HTTP серевер
  HTTPhandle_h = &HTTPhandle;
  // Включаем работу с файловой системой
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    String fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
  };
}

//Методы
//Чтение файла
bool SRfs::handleFileRead(String path) {
  if (path.endsWith("/"))
    path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
    if (SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = HTTPhandle_h->streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

//Загрузка файла
void SRfs::handleFileUpload() {
  if (HTTPhandle_h->uri() != "/edit")
    return;
  HTTPUpload &upload = HTTPhandle_h->upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/"))
      filename = "/" + filename;
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    // DBG_OUTPUT_PORT.print("handleFileUpload Data: ");
    // DBG_OUTPUT_PORT.println(upload.currentSize);
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile)
      fsUploadFile.close();
  }
}

//Удаление файла
void SRfs::handleFileDelete() {
  if (HTTPhandle_h->args() == 0)
    return HTTPhandle_h->send(500, "text/plain", "BAD ARGS");
  String path = HTTPhandle_h->arg(0);
  if (path == "/")
    return HTTPhandle_h->send(500, "text/plain", "BAD PATH");
  if (!SPIFFS.exists(path))
    return HTTPhandle_h->send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  HTTPhandle_h->send(200, "text/plain", "");
  path = String();
}

//Создание файла
void SRfs::handleFileCreate() {
  if (HTTPhandle_h->args() == 0)
    return HTTPhandle_h->send(500, "text/plain", "BAD ARGS");
  String path = HTTPhandle_h->arg(0);
  if (path == "/")
    return HTTPhandle_h->send(500, "text/plain", "BAD PATH");
  if (SPIFFS.exists(path))
    return HTTPhandle_h->send(500, "text/plain", "FILE EXISTS");
  File file = SPIFFS.open(path, "w");
  if (file)
    file.close();
  else
    return HTTPhandle_h->send(500, "text/plain", "CREATE FAILED");
  HTTPhandle_h->send(200, "text/plain", "");
  path = String();
}

//получение списка файлов
void SRfs::handleFileList() {

  if (!HTTPhandle_h->hasArg("dir")) {
    HTTPhandle_h->send(500, "text/plain", "BAD ARGS");
    return;
  }
  String path = HTTPhandle_h->arg("dir");
  Dir dir = SPIFFS.openDir(path);
  path = String();
  String output = "[";
  while (dir.next()) {
    File entry = dir.openFile("r");
    if (output != "[")
      output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir) ? "dir" : "file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }
  output += "]";

  HTTPhandle_h->send(200, "text/json", output);
}

//Приватные функции
String SRfs::getContentType(String filename) {
  if (HTTPhandle_h->hasArg("download"))
    return "application/octet-stream";
  else if (filename.endsWith(".htm"))
    return "text/html";
  else if (filename.endsWith(".json"))
    return "application/json";
  else if (filename.endsWith(".html"))
    return "text/html";
  else if (filename.endsWith(".css"))
    return "text/css";
  else if (filename.endsWith(".js"))
    return "application/javascript";
  else if (filename.endsWith(".png"))
    return "image/png";
  else if (filename.endsWith(".gif"))
    return "image/gif";
  else if (filename.endsWith(".jpg"))
    return "image/jpeg";
  else if (filename.endsWith(".ico"))
    return "image/x-icon";
  else if (filename.endsWith(".xml"))
    return "text/xml";
  else if (filename.endsWith(".pdf"))
    return "application/x-pdf";
  else if (filename.endsWith(".zip"))
    return "application/x-zip";
  else if (filename.endsWith(".gz"))
    return "application/x-gzip";
  return "text/plain";
}
