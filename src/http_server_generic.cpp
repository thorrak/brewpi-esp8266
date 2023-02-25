#ifdef ENABLE_HTTP_INTERFACE

#include <Arduino.h>
#include <ArduinoJson.h>
#include "ESPEepromAccess.h"

#include "http_server.h"



// Functions needed to serve static files
String httpServer::getContentType(String filename) {
  if (web_server->hasArg("download")) {
    return "application/octet-stream";
  } else if (filename.endsWith(".htm")) {
    return "text/html";
  } else if (filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".png")) {
    return "image/png";
  } else if (filename.endsWith(".gif")) {
    return "image/gif";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".ico")) {
    return "image/x-icon";
  } else if (filename.endsWith(".xml")) {
    return "text/xml";
  } else if (filename.endsWith(".pdf")) {
    return "application/x-pdf";
  } else if (filename.endsWith(".zip")) {
    return "application/x-zip";
  } else if (filename.endsWith(".gz")) {
    return "application/x-gzip";
  } else if (filename.endsWith(".svg")) {
    return "image/svg+xml";
  }
  return "text/plain";
}

bool httpServer::exists(String path){
  bool yes = false;
  File file = FILESYSTEM.open(path, "r");
  if(!file.isDirectory()){
    yes = true;
  }
  file.close();
  return yes;
}

bool httpServer::handleFileRead(String path) {
  // Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) {
    path += "index.htm";
  }
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (exists(pathWithGz) || exists(path)) {
    if (exists(pathWithGz)) {
      path += ".gz";
    }
    File file = FILESYSTEM.open(path, "r");
    web_server->streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void httpServer::redirect(const String& url){
    web_server->sendHeader("Location", url);
    web_server->sendHeader("Cache-Control", "no-cache");
    web_server->send(302);
    return;
}

#endif // ENABLE_HTTP_INTERFACE
