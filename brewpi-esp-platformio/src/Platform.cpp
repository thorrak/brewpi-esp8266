
#undef min
#undef max

#include <FS.h>  // Apparently this needs to be first
#include <EEPROM.h>				//For storing the configuration constants
#include "Brewpi.h"
//#include "application.h"
#include "EepromManager.h"

#ifdef ESP8266_WiFi_Control
#include "ESP8266mDNS.h"
#include <ESP8266WiFi.h>		//ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <DNSServer.h>			//Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>	//Local WebServer used to serve the configuration portal
#include <WiFiManager.h>		//https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include "ESP8266mDNS.h"
#endif

void handleReset()
{
	// The asm volatile method doesn't work on ESP8266. Instead, use ESP.restart
	ESP.restart();
}

void flashFirmware()
{
	// NOOP
}


/////////////// WIFI SPECIFIC STUFF /////////////////

#ifdef ESP8266_WiFi
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback() {
	Serial.println("Should save config");
	shouldSaveConfig = true;
}

// Not sure if this is sufficient to test for validity
bool isValidmDNSName(String mdns_name) {
	for (std::string::size_type i = 0; i < mdns_name.length(); ++i) {
		// For now, we're just checking that every character in the string is alphanumeric. May need to add more validation here.
		if (!isalnum(mdns_name[i]))
			return false;
	}
	return true;
}

WiFiServer server(23);
WiFiClient serverClient;

void connectClients() {
	if (server.hasClient()) {
		if (!serverClient || !serverClient.connected()) {
			if (serverClient) serverClient.stop();
			serverClient = server.available();
		}
		else {
			//no free/disconnected spot so reject
			WiFiClient rejectClient = server.available();
			rejectClient.stop();
		}
	}
}

#endif

///////////// END WIFI SPECIFIC STUFF /////////////////




bool platform_init()
{

#ifdef ESP8266_WiFi
	String mdns_id;

	// The below loads the mDNS name from the file we saved it to (if the file exists)
	if (SPIFFS.begin()) {
		if (SPIFFS.exists("/mdns.txt")) {
			// The file exists - load it up
			File dns_name_file = SPIFFS.open("/mdns.txt", "r");
			if (dns_name_file) {
				// Assuming everything goes well, read in the mdns name
				mdns_id = dns_name_file.readStringUntil('\n');
			} else {
				// The file exists, but we weren't able to read from it
				mdns_id = "ESP" + String(ESP.getChipId());
			}
		} else {
			// The file doesn't exist	
			mdns_id = "ESP" + String(ESP.getChipId());
		}
	} else {
		// There's some kind of issue with SPIFFS.
        logErrorString(ERROR_SPIFFS_FAILURE, "/mdns.txt".c_str());
		mdns_id = "ESP" + String(ESP.getChipId());
	}
	mdns_id.trim();


	// If we're going to set up WiFi, let's get to it
	WiFiManager wifiManager;
	wifiManager.setConfigPortalTimeout(5 * 60); // Time out after 5 minutes so that we can keep managing temps 
	wifiManager.setDebugOutput(false); // In case we have a serial connection to BrewPi

									   // The main purpose of this is to set a boolean value which will allow us to know we
									   // just saved a new configuration (as opposed to rebooting normally)
	wifiManager.setSaveConfigCallback(saveConfigCallback);

	// The third parameter we're passing here (mdns_id.c_str()) is the default name that will appear on the form.
	// It's nice, but it means the user gets no actual prompt for what they're entering. 
	WiFiManagerParameter custom_mdns_name("mdns", "Device (mDNS) Name", mdns_id.c_str(), 20);
	wifiManager.addParameter(&custom_mdns_name);

	wifiManager.autoConnect(); // Launch captive portal with auto generated name ESP + ChipID

							   // Alright. We're theoretically connected here (or we timed out).
							   // If we connected, then let's save the mDNS name
	if (shouldSaveConfig) {
		// If the mDNS name is valid, save it.
		if (isValidmDNSName(custom_mdns_name.getValue())) {
			File dns_name_file = SPIFFS.open("/mdns.txt", "w");
			if (dns_name_file) {
				// If the above fails, we weren't able to open the file for writing
				mdns_id = custom_mdns_name.getValue();
				dns_name_file.println(mdns_id);
			}
			dns_name_file.close();
		}
		else {
			// If the mDNS name is invalid, reset the WiFi configuration and restart the ESP8266
			WiFi.disconnect(true);
			delay(2000);
			handleReset();
		}
	}

	// Regardless of the above, we need to set the mDNS name and announce it
	if (!MDNS.begin(mdns_id.c_str())) {
		// TODO - Do something about it or log it or something
	}

#endif

    bool initialize = !eepromManager.hasSettings();
    if(initialize) {
        eepromManager.zapEeprom();  // Writes all the empty files to SPIFFS
    }

	return initialize;
}

// Apparently lldiv isn't implemented on ESP8266. Go figure.
lldiv_t ICACHE_FLASH_ATTR _DEFUN(lldiv, (number, denom),
	long long numer _AND long long denom)
{
	lldiv_t retval;

	retval.quot = numer / denom;
	retval.rem = numer % denom;
	if (numer >= 0 && retval.rem < 0) {
		retval.quot++;
		retval.rem -= denom;
	}
	return (retval);
}


size_t _DEFUN(wcstombs, (s, pwcs, n),
	char          *__restrict s    _AND
	const wchar_t *__restrict pwcs _AND
	size_t         n)
{
#ifdef _MB_CAPABLE
	mbstate_t state;
	state.__count = 0;

	return _wcstombs_r(_REENT, s, pwcs, n, &state);
#else /* not _MB_CAPABLE */
	int count = 0;

	if (n != 0) {
		do {
			if ((*s++ = (char)*pwcs++) == 0)
				break;
			count++;
		} while (--n != 0);
	}

	return count;
#endif /* not _MB_CAPABLE */
}

