
#include "JsonMessages.h"
#include "Brewpi.h"
#include "Version.h"
#include "Display.h"
#include "TempControl.h"
#include "TemperatureFormats.h"




void versionInfoJson(DynamicJsonDocument &doc) {
  // v version
  // s shield type
  // y: simulator
  // b: board
  // e: 

  doc["v"] = Config::Version::release;
  doc["n"] = Config::Version::git_rev;
  doc["c"] = Config::Version::git_tag;
  doc["s"] = BREWPI_STATIC_CONFIG;
  doc["y"] = BREWPI_SIMULATE;
  doc["b"] = String(BREWPI_BOARD);
  doc["l"] = BREWPI_LOG_MESSAGES_VERSION;
  doc["e"] = FIRMWARE_REVISION;

}


void getLcdContentJson(DynamicJsonDocument &doc) {
  JsonArray rootArray = doc.to<JsonArray>();
  char stringBuffer[Config::Lcd::columns + 2];

  for (uint8_t i = 0; i < Config::Lcd::lines; i++) {
    display.getLine(i, stringBuffer);
    rootArray.add(stringBuffer);
  }
}


void temp_with_null(DynamicJsonDocument &doc, const char* key, bool sensorConnected, temperature temp, bool withNull) {
  if (sensorConnected) {
    doc[key] = tempToDouble(temp, Config::TempFormat::tempDecimals);
  } else {
    if (withNull) {
      doc[key] = "";
    }
  }
}

void printTemperaturesJson(DynamicJsonDocument &doc, const char *beerAnnotation, const char *fridgeAnnotation, bool withNulls) {

    temp_with_null(doc, "BeerTemp", tempControl.beerSensor->isConnected(), tempControl.getBeerTemp(), withNulls);
    // TODO - fix BeerSet to track if we actually have a setpoint
    temp_with_null(doc, "BeerSet", tempControl.beerSensor->isConnected(), tempControl.getBeerSetting(), withNulls);
    temp_with_null(doc, "FridgeTemp", tempControl.fridgeSensor->isConnected(), tempControl.getFridgeTemp(), withNulls);
    // TODO - fix FridgeSet to track if we actually have a setpoint
    temp_with_null(doc, "FridgeSet", tempControl.fridgeSensor->isConnected(), tempControl.getFridgeSetting(), withNulls);
    temp_with_null(doc, "RoomTemp", tempControl.ambientSensor->isConnected(), tempControl.getRoomTemp(), true);

    // doc["BeerTemp"] = tempToDouble(tempControl.getBeerTemp(), Config::TempFormat::tempDecimals);
    // doc["BeerSet"] = tempToDouble(tempControl.getBeerSetting(), Config::TempFormat::tempDecimals);

    doc["BeerAnn"] = beerAnnotation;

    // doc["FridgeTemp"] = tempToDouble(tempControl.getFridgeTemp(), Config::TempFormat::tempDecimals);
    // doc["FridgeSet"] = tempToDouble(tempControl.getFridgeSetting(), Config::TempFormat::tempDecimals);

    doc["FridgeAnn"] = fridgeAnnotation;

    // if (tempControl.ambientSensor->isConnected()) {
    //   doc["RoomTemp"] = tempToDouble(tempControl.getRoomTemp(), Config::TempFormat::tempDecimals);
    // } else {
    //   doc["RoomTemp"] = "";
    // }

    doc["State"] = tempControl.getState();

#if BREWPI_SIMULATE
    doc["Time"] = ticks.millis() / 1000;
#endif
}

void printTemperaturesJson(DynamicJsonDocument &doc, const char *beerAnnotation, const char *fridgeAnnotation) {
   printTemperaturesJson(doc, beerAnnotation, fridgeAnnotation, false);
}

void printTemperaturesJson(DynamicJsonDocument &doc) {
    printTemperaturesJson(doc, "", "");
}

void getFullTemperatureControlJson(DynamicJsonDocument &doc) {
  DynamicJsonDocument cc(256);
  DynamicJsonDocument cs(256);
  DynamicJsonDocument cv(256);
  DynamicJsonDocument temp(1024);

  tempControl.getControlConstantsDoc(cc);
  tempControl.getControlSettingsDoc(cs);
  tempControl.getControlVariablesDoc(cv);
  printTemperaturesJson(temp);

  // For this message, we don't want to send any temperature if the sensor is not connected
  if(!tempControl.beerSensor->isConnected())
      temp["BeerTemp"] = "";
  if(!tempControl.fridgeSensor->isConnected())
      temp["FridgeTemp"] = "";

  doc["cc"] = cc;
  doc["cs"] = cs;
  doc["cv"] = cv;
  doc["temp"] = temp;
}
