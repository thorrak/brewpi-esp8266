#pragma once

/**
 * \brief Provide a PiLink interface compatible with the previous API
 *
 * This serves as an adaptor until other uses can be updated to use the
 * JsonDocument style interface.  Once everything is converted, this template
 * can be removed.
 */
template <typename StreamType>
class CompatiblePiLink : public PiLink<StreamType> {
  static_assert(std::is_base_of<Stream, StreamType>::value,
                "StreamType must be a Stream");

public:
  CompatiblePiLink(StreamType &stream) : PiLink<StreamType>(stream) {}

  typedef void (*ParseJsonCallback)(const char *key, const char *val,
                                    void *data);

  /**
   * @brief Parse JSON keypairs
   *
   * Calls fn for each pair
   * @deprecated Use the JsonDocument interface in PiLink
   */
  void parseJson(ParseJsonCallback fn, void *data = NULL)
      __attribute__((deprecated)) {

    StaticJsonDocument<200> doc;
    this->receiveJsonMessage(doc);

    // Loop over every key, call the call back for each
    JsonObject root = doc.as<JsonObject>();
    for (JsonPair kv : root) {
      fn(kv.key().c_str(), kv.value().as<char *>(), data);
    }
  }

  /**
   * @brief Print the identifier header before a response
   *
   * The header is used by the client end to identify the type of data it is
   * about to receive.
   *
   * @deprecated Use the JsonDocument interface in PiLink
   */
  void printResponse(char type) __attribute__((deprecated)) {
    this->print_fmt("%c:", type);
  }

  void sendJsonClose() __attribute__((deprecated)) {
    this->print('}');
    this->printNewLine();
  }

  /**
   * @brief Begin a response that is made up of a list
   *
   * @see printResponse
   * @deprecated Use the JsonDocument interface in PiLink
   */
  void openListResponse(char type) __attribute__((deprecated)) {
    printResponse(type);
    this->print('[');
  }

  /**
   * @brief End a JSON list response
   *
   * @see openListResponse
   * @deprecated Use the JsonDocument interface in PiLink
   */
  void closeListResponse() __attribute__((deprecated)) {
    this->print(']');
    this->printNewLine();
  }
};
