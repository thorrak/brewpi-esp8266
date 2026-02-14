#ifdef ENABLE_HTTP_INTERFACE

#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoLog.h>

#include "ESPEepromAccess.h"

#include "http_server.h"



// Functions needed to serve static files
String httpServer::getContentType(String filename) {
  if (filename.endsWith(".htm")) return "text/html";
  if (filename.endsWith(".html")) return "text/html";
  if (filename.endsWith(".css")) return "text/css";
  if (filename.endsWith(".js")) return "application/javascript";
  if (filename.endsWith(".png")) return "image/png";
  if (filename.endsWith(".gif")) return "image/gif";
  if (filename.endsWith(".jpg")) return "image/jpeg";
  if (filename.endsWith(".ico")) return "image/x-icon";
  if (filename.endsWith(".xml")) return "text/xml";
  if (filename.endsWith(".pdf")) return "application/x-pdf";
  if (filename.endsWith(".zip")) return "application/x-zip";
  if (filename.endsWith(".gz")) return "application/x-gzip";
  if (filename.endsWith(".svg")) return "image/svg+xml";
  return "text/plain";
}

bool httpServer::handleFileRead(AsyncWebServerRequest *request, String path) {
  if (path.endsWith("/")) {
      path += "index.html";
  }
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (FILESYSTEM.exists(pathWithGz) || FILESYSTEM.exists(path)) {
      AsyncWebServerResponse* response = request->beginResponse(FILESYSTEM, path, contentType);
      if (FILESYSTEM.exists(pathWithGz)) {
          path += ".gz";
          response->addHeader("Content-Encoding", "gzip");
      }
      request->send(response);
      return true;
  }
  return false;
}

void httpServer::redirect(AsyncWebServerRequest *request, const String &url) {
  request->redirect(url);
}


#endif // ENABLE_HTTP_INTERFACE
