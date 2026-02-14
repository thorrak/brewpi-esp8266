#include <ArduinoJson.h>



void versionInfoJson(JsonDocument &doc);
void getLcdContentJson(JsonDocument &doc);
void printTemperaturesJson(JsonDocument &doc, const char *beerAnnotation, const char *fridgeAnnotation);
void printTemperaturesJson(JsonDocument &doc, const char *beerAnnotation, const char *fridgeAnnotation, bool withNulls);
void printTemperaturesJson(JsonDocument &doc);
void getFullTemperatureControlJson(JsonDocument &doc);

