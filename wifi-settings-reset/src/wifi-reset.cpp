
#include <FS.h>  // Apparently this needs to be first
#include <ESP8266WiFi.h>



void setup()
{

    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println();
    Serial.print("Disconnecting WiFi... ");
    WiFi.disconnect(true);
    delay(2000);

    if(WiFi.status() != WL_CONNECTED) {
        Serial.print("...disconnected!");
    }

    Serial.print("\r\n\r\nResetting SPIFFS...\r\n");

    SPIFFS.begin();
    // Next lines have to be done ONLY ONCE!!!!!When SPIFFS is formatted ONCE you can comment these lines out!!
    Serial.println("Please wait 30 secs for SPIFFS to be formatted");
    SPIFFS.format();
    Serial.println("Spiffs formatted!\r\n");

    Serial.println("Please disconnect your ESP8266 and reflash with the final firmware.");

}

void loop()
{
    // Do nothing.
    delay(2000);

}