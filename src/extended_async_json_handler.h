/**
 * @file ExtendedAsyncCallbackJsonWebHandler.h
 * @brief A class for handling JSON requests in AsyncWebServer with a custom handler function.
 */

 #ifndef EXTENDED_ASYNC_JSON_HANDLER_H_
 #define EXTENDED_ASYNC_JSON_HANDLER_H_
 
 #include <AsyncJson.h>
 
 #if ASYNC_JSON_SUPPORT == 1
 
 /**
  * @class ExtendedAsyncCallbackJsonWebHandler
  * @brief Extends AsyncCallbackJsonWebHandler to support custom JSON request handling.
  *
  * This class provides the ability to process incoming JSON requests in an `AsyncWebServer`
  * using a user-defined handler function. The handler processes the JSON payload and returns
  * a success or failure response based on its logic.
  */
 class ExtendedAsyncCallbackJsonWebHandler : public AsyncCallbackJsonWebHandler {
   protected:
     /**
      * @brief Pointer to the custom handler function.
      *
      * The custom handler function processes the JSON payload and determines whether the
      * request should be considered successful or not.
      */
     bool (*_customHandler)(const JsonDocument&, bool);
 
   public:
     /**
      * @brief Constructor for ExtendedAsyncCallbackJsonWebHandler.
      *
      * @param uri The URI to handle.
      * @param maxJsonBufferSize Maximum size of the JSON buffer (defaults to DYNAMIC_JSON_DOCUMENT_SIZE).
      * @param customHandler A pointer to the custom handler function. The function must accept
      *        a `JsonDocument` and a `bool`, and return a `bool` indicating success.
      *
      * The custom handler function will be invoked for each request to the specified URI. If
      * the handler is not provided or fails, an appropriate HTTP response will be sent to the client.
      */
     ExtendedAsyncCallbackJsonWebHandler(
         const char* uri,
         bool (*customHandler)(const JsonDocument&, bool) = nullptr)
       : AsyncCallbackJsonWebHandler(
             uri,
             [customHandler](AsyncWebServerRequest* request, JsonVariant& json) {
                 if (!customHandler) {
                     request->send(500, "application/json", "{\"error\":\"No handler provided\"}");
                     return;
                 }
 
                 // Parse the JSON payload
                 JsonDocument doc;
                 doc = json.as<JsonObject>();
 
                 // Call the handler
                 if (customHandler(doc, true)) {
                     request->send(200, "application/json", "{\"status\":\"ok\"}");
                 } else {
                     request->send(400, "application/json", "{\"status\":\"error\"}");
                 }
             }),
         _customHandler(customHandler) {}
 };
 
 #endif // ASYNC_JSON_SUPPORT == 1
 
 #endif // EXTENDED_ASYNC_JSON_HANDLER_H_
 