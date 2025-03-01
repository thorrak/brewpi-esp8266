/**
 * @file extended_async_json_handler.h
 * @brief A class for handling JSON requests in AsyncWebServer with a custom handler function.
 */

 #ifndef EXTENDED_ASYNC_JSON_HANDLER_H_
 #define EXTENDED_ASYNC_JSON_HANDLER_H_
 
 #include <AsyncJson.h>
 
 #if ASYNC_JSON_SUPPORT == 1
 
/**
 * @class PutAsyncCallbackJsonWebHandler
 * @brief Extends AsyncCallbackJsonWebHandler to support custom JSON request handling for HTTP PUT.
 *
 * This class provides the ability to process incoming PUT requests in an `AsyncWebServer`
 * using a user-defined handler function. The handler processes the JSON payload and returns
 * a success or failure response based on its logic.
 */
 class PutAsyncCallbackJsonWebHandler : public AsyncCallbackJsonWebHandler {
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
      * @brief Constructor for PutAsyncCallbackJsonWebHandler.
      *
      * @param uri The URI to handle.
      * @param maxJsonBufferSize Maximum size of the JSON buffer (defaults to DYNAMIC_JSON_DOCUMENT_SIZE).
      * @param customHandler A pointer to the custom handler function. The function must accept
      *        a `JsonDocument` and a `bool`, and return a `bool` indicating success.
      *
      * The custom handler function will be invoked for each request to the specified URI. If
      * the handler is not provided or fails, an appropriate HTTP response will be sent to the client.
      */
     PutAsyncCallbackJsonWebHandler(
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
         _customHandler(customHandler) {
          setMethod(HTTP_PUT);
         }
 };


/**
 * @class GetAsyncCallbackJsonWebHandler
 * @brief Extends AsyncCallbackJsonWebHandler to support custom JSON response handling for HTTP GET.
 *
 * This class provides the ability to process incoming GET requests in an `AsyncWebServer`
 * using a user-defined handler function that generates and returns JSON data.
 */
class GetAsyncCallbackJsonWebHandler : public AsyncCallbackJsonWebHandler {
  protected:
  /**
   * @brief Pointer to the custom handler function.
   *
   * The custom handler function generates JSON data to be sent in the response.
   * It accepts a `JsonDocument` reference which it populates with the response data.
   */
  void (*_customHandler)(JsonDocument&);
  
  public:
      /**
       * @brief Constructor for GetAsyncCallbackJsonWebHandler.
       *
       * @param uri The URI to handle.
       * @param customHandler A pointer to the custom handler function. The function must accept
       *        a `JsonDocument` reference and populate it with the response data.
       *
       * The custom handler function will be invoked for each GET request to the specified URI.
       * If the handler is not provided, an appropriate HTTP response will be sent to the client.
       */
      GetAsyncCallbackJsonWebHandler(
        const char* uri,
        void (*customHandler)(JsonDocument&) = nullptr)
      : AsyncCallbackJsonWebHandler(
            uri,
            [customHandler](AsyncWebServerRequest* request, JsonVariant& json) {
                if (!customHandler) {
                    request->send(500, "application/json", "{\"error\":\"No handler provided\"}");
                    return;
                }

                AsyncJsonResponse *response = new AsyncJsonResponse();
                {
                  JsonDocument doc;
                  customHandler(doc);
              
                  // // Print the contents of doc to the serial console
                  // Serial.println(F("Generated JSON:"));
                  // serializeJsonPretty(doc, Serial); // Pretty print for easier reading
                  // Serial.println(); // Add a newline for better formatting
              
                  response->getRoot().set(doc);
                }
              
                response->setLength();
                request->send(response);

            }),
        _customHandler(customHandler) {
          setMethod(HTTP_GET);
        }

  };
 
 #endif // ASYNC_JSON_SUPPORT == 1
 
 #endif // EXTENDED_ASYNC_JSON_HANDLER_H_
 