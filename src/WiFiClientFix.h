#pragma once

#include <WiFiClient.h>
#include <WiFiClientSecure.h>

class WiFiClientFixed : public WiFiClient {
public:
    WiFiClientFixed();
    WiFiClientFixed(int fd);
    ~WiFiClientFixed();
	void flush() override;
};

class WiFiClientSecureFixed : public WiFiClientSecure {
public:
	void flush() override;
};
