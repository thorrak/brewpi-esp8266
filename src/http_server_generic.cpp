#ifdef ENABLE_HTTP_INTERFACE

#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoLog.h>
#include <cstring>

#include "ESPEepromAccess.h"

#include "http_server.h"


// Helper to check if a C-string ends with a given suffix
static bool endsWith(const char* str, const char* suffix) {
    size_t strLen = strlen(str);
    size_t suffixLen = strlen(suffix);
    if (suffixLen > strLen) return false;
    return strcmp(str + strLen - suffixLen, suffix) == 0;
}

// Functions needed to serve static files
const char* httpServer::getContentType(const char* filename) {
  if (endsWith(filename, ".htm")) return "text/html";
  if (endsWith(filename, ".html")) return "text/html";
  if (endsWith(filename, ".css")) return "text/css";
  if (endsWith(filename, ".js")) return "application/javascript";
  if (endsWith(filename, ".png")) return "image/png";
  if (endsWith(filename, ".gif")) return "image/gif";
  if (endsWith(filename, ".jpg")) return "image/jpeg";
  if (endsWith(filename, ".ico")) return "image/x-icon";
  if (endsWith(filename, ".xml")) return "text/xml";
  if (endsWith(filename, ".pdf")) return "application/x-pdf";
  if (endsWith(filename, ".zip")) return "application/x-zip";
  if (endsWith(filename, ".gz")) return "application/x-gzip";
  if (endsWith(filename, ".svg")) return "image/svg+xml";
  return "text/plain";
}

bool httpServer::handleFileRead(AsyncWebServerRequest *request, const char* path) {
  char fullPath[256];
  strlcpy(fullPath, path, sizeof(fullPath));

  size_t len = strlen(fullPath);
  if (len > 0 && fullPath[len - 1] == '/') {
      strlcat(fullPath, "index.html", sizeof(fullPath));
  }

  const char* contentType = getContentType(fullPath);

  char pathWithGz[260];
  snprintf(pathWithGz, sizeof(pathWithGz), "%s.gz", fullPath);

  if (FILESYSTEM.exists(pathWithGz) || FILESYSTEM.exists(fullPath)) {
      AsyncWebServerResponse* response = request->beginResponse(FILESYSTEM, fullPath, contentType);
      if (FILESYSTEM.exists(pathWithGz)) {
          strlcat(fullPath, ".gz", sizeof(fullPath));
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
