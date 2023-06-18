#include <ArduinoJson.h>



void versionInfoJson(DynamicJsonDocument &doc);
void getLcdContentJson(DynamicJsonDocument &doc);
void printTemperaturesJson(DynamicJsonDocument &doc, const char *beerAnnotation, const char *fridgeAnnotation);
void printTemperaturesJson(DynamicJsonDocument &doc, const char *beerAnnotation, const char *fridgeAnnotation, bool withNulls);
void printTemperaturesJson(DynamicJsonDocument &doc);
void getFullTemperatureControlJson(DynamicJsonDocument &doc);

