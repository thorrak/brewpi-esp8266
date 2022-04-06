/*
 * Copyright 2012-2013 BrewPi/Elco Jacobs.
 * Copyright 2013 Matthew McGowan.
 * Copyright 2020 Scott Peshak.
 *
 * This file is part of BrewPi.
 *
 * BrewPi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * BrewPi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with BrewPi.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "Brewpi.h"
#include <ArduinoJson.h>
#include <StreamUtils.h>
#include <type_traits>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

#include "ESP_BP_WiFi.h"

/**
 * \brief Template for abstracting low level Stream interaction details
 *
 * This encapsulates the differences required when using different Stream
 * implementations.  Writes are buffered when Config::PiLink::bufferPrints is
 * true.  StreamType must be something that implements the Stream API (like
 * Serial) The default is written for Serial.  See specializations for other
 * implementations
 */
template <typename StreamType> class PiStream {
  static_assert(std::is_base_of<Stream, StreamType>::value, "StreamType must be a Stream");

public:
  /**
   * \brief Constructor
   *
   * \param stream - Reference to the Stream object used for communications
   */
  PiStream(StreamType &stream) : intBuffOn(0), upstream(stream), stream{stream, Config::PiLink::printBufferSize()} {};

  /**
   * \brief Read data from the stream
   */
  int read() { return stream.read(); };

  /**
   * \brief Read from the Stream, waiting if nothing is yet available to read
   *
   * If data isn't immediately available, continue to poll until some data
   * is.  The stream will be polled for data every millisecond, up until the
   * timeout.
   *
   * \return Data from Stream, or if no data is received before timeout, -1
   * \param timeout - How long to wait for data, in milliseconds
   */
  int readPersistent(const int timeout = 10) {
    uint8_t retries = 0;
    while (available() == 0) {
      // Uses delay as delayMicroseconds doesn't yield like delay does
      delay(1);
      yield();
      retries++;
      if (retries >= timeout) {
        return -1;
      }
    }
    return stream.read();
  };

  /**
   * \brief Print a single char
   */
  void print(const char out) { 
    if(Config::PiLink::bufferPrints) {
      stream.print(out); 
    } else {
      if((intBuffOn + 1)  < Config::PiLink::intBufferSize()) {
        intBuff[intBuffOn] = out;
        intBuff[intBuffOn+1] = '\0';
        intBuffOn++;
      }
    }
    
  };

  /**
   * \brief Print a C-str
   */
  void print(const char *out) { 
    if(Config::PiLink::bufferPrints) {
      stream.print(out); 
    } else {
      for(uint16_t x = 0; x < strlen(out) ; x++) {
        if((intBuffOn + 1)  < Config::PiLink::intBufferSize()) {
          intBuff[intBuffOn] = out[x];
          intBuff[intBuffOn+1] = '\0';
          intBuffOn++;
        }
      }
    }
  };

  /**
   * \brief Print a String
   */
  void print(const String out) { print(out.c_str()); };

  /**
   * \brief Print a C++ String
   */
  void print(const std::string out) { print(out.c_str()); };

  /**
   * \brief A printf like interface.  Format string stored in PROGMEM
   *
   * \param fmt - PROGMEM stored sprintf format string
   */
  void print_P(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf_P(printfBuff, Config::PiLink::printfBufferSize, fmt, args);
    va_end(args);

    print(printfBuff);
  };

  /**
   * \brief A printf like interface to the Arduino Serial function. Format
   * string stored in RAM
   *
   * This was originally named print, but it is ambiguous with the other
   * print() methods (because the variadic could match 0 args, which then
   * looks the same as print(const char*)
   *
   * I was going to use a [template
   * solution](https://stackoverflow.com/questions/41860736/how-to-disambiguate-a-variadic-function-with-another-in-c)
   * to disambiguate this from print(), but there is a [bug in gcc before
   * v4.9](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=41933) that prevents
   * that kind of template from working.  As a cheap workaround, I'm renaming
   * the method.  If the toolchain gets updated in the future, revisiting the
   * template would be a good idea.
   *
   * \param fmt - sprintf format string
   */
  void print_fmt(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(printfBuff, Config::PiLink::printfBufferSize, fmt, args);
    va_end(args);

    print(printfBuff);
  };

  /**
   * \brief Print new line
   *
   * Flushes the buffer, if buffering was enabled.
   */
  void printNewLine() {

    if(Config::PiLink::bufferPrints) {
      stream.println();
      stream.flush();  // This currently causes a crash on ESP32, and is not required if bufferPrints is disabled
    } else {
      stream.println(intBuff);
      intBuff[0] = '\0';
      intBuffOn = 0;
    }

  };

  /**
   * \brief Check if stream is conected
   */
  bool connected() { return upstream.connected(); };

  /**
   * \brief Check if data is available for reading
   */
  bool available() { return stream.available(); };

  /**
   * \brief Send a JSON document with an optional prefix
   *
   * \param prefix - Used to indicate to the receiving end the nature of the
   * message
   * \param doc - Reference to JsonDocument that should be sent
   */
  void sendJsonMessage(const char prefix, const JsonDocument &doc) {
    if (prefix) {
      print(prefix);
      print(':');
    }

    if(Config::PiLink::bufferPrints) {
      serializeJson(doc, stream);
    } else {
      char buf[2048];
      serializeJson(doc, buf, 2048);
      print(buf);
    }
    printNewLine();
  }

  /**
   * \brief Unpack a single item array
   *
   * This allows client code to always generate arrays, even if there is only
   * going to be a single object.  This is used to unpack that single object
   * into the top level root before serializing over the stream.
   *
   * \param prefix - Used to indicate to the receiving end the nature of the
   * message
   * \param doc - Reference to JsonDocument that should be sent
   */
  void sendSingleItemJsonMessage(const char prefix, JsonDocument &doc) {
    if (prefix) {
      print(prefix);
      print(':');
    }

    // Pull the device object from the document array and promote it to be root
    DynamicJsonDocument shallowDoc(doc.capacity());

    for (auto kvp : doc.as<JsonArray>()[0].as<JsonObject>()) {
      shallowDoc[kvp.key()] = kvp.value();
    }

    serializeJson(shallowDoc, stream);
    printNewLine();
  }

  /**
   * \brief Parse JSON coming from the stream
   *
   * \param doc - Reference to a JsonDocument to populate.
   */
  void receiveJsonMessage(JsonDocument &doc) {
    const DeserializationError error = deserializeJson(doc, stream);

    if (error) {
      print("Error deserializing JSON data ");
      print(error.c_str());
      printNewLine();
    }
  }

  /**
   * \brief bool operator
   *
   * This dispatches to a private _bool() method for easier templating
   * @see _bool()
   */
  operator bool() { return _bool(); };

  /**
   * \brief Empty init for all other stream types types
   */
  template <typename U = StreamType>
  typename std::enable_if<!std::is_same<U, HardwareSerial>::value && !std::is_same<U, WiFiClient>::value>::type
  init(){};

  /**
   * \brief Initialize WiFi Stream
   *
   * \see initWifiServer()
   */
  template <typename U = StreamType> typename std::enable_if<std::is_same<U, WiFiClient>::value>::type init() {
    ::initWifiServer();
  };

  /**
   * \brief Initialize Serial Stream
   */
  template <typename U = StreamType> typename std::enable_if<std::is_same<U, HardwareSerial>::value>::type init() {
    upstream.begin(Config::PiLink::serialSpeed);
  };

private:
  /**
   * \brief Buffer used for printf operations
   * \see Config::PiLink::printfBufferSize
   */
  static char printfBuff[Config::PiLink::printfBufferSize];

  /**
   * \brief Buffer used for printf operations
   * \see Config::PiLink::printfBufferSize
   */
  char intBuff[Config::PiLink::intBufferSize()];
  uint16_t intBuffOn;

  /**
   * \brief Reference to the wrapped StreamType object
   *
   * This is used for type specific interactions.  Generic Stream usage is
   * done via stream;
   */
  StreamType &upstream;

  /**
   * \brief Stream used for communications
   *
   * \todo
   * To aid debugging, we could enable the ReadLoggingStream feature of
   * StreamUtils and have it dump the content to the serial interface when
   * we're in WiFi mode (because it shouldn't cause any interfearance with
   * real comms).  The docs for DerserializationError has an example
   * (https://arduinojson.org/v6/api/misc/deserializationerror/)
   *
   * This may be tricky to combine with the write buffering, that's why I haven't done it yet.
   */
  // WriteBufferingStream stream;
  WriteBufferingClient stream;

  /**
   * \brief _bool implementation for non WiFiClient Streams
   */
  template <typename U = StreamType> typename std::enable_if<!std::is_same<U, WiFiClient>::value, bool>::type _bool() {
    return bool(stream);
  };

  /**
   * \brief _bool implementation for WiFiClient Streams
   *
   * This checks both the bool operator of the underlying Stream and its
   * connected() method
   */
  template <typename U = StreamType> typename std::enable_if<std::is_same<U, WiFiClient>::value, bool>::type _bool() {
    return bool(stream) && upstream.connected();
  };
};

template <typename T> char PiStream<T>::printfBuff[Config::PiLink::printfBufferSize];
