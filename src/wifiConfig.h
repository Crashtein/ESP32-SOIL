#pragma once
#ifndef AP_SSID
#define AP_SSID ESP32
#endif
#ifndef AP_PASSWORD
#define AP_PASSWORD NULL
#endif

#include <WiFiManager.h>
#include "Config.h"


class WifiConfig
{
public:
    static WifiConfig &getInstance();
    void setup();
    void startPortal();
    void reset();
    bool getStatus();
    void setAPCallback(void APcallback(WiFiManager *));

private:
    WifiConfig();
    WiFiManager wifiManager;
    WiFiManagerParameter *custom_mqtt_server;
    WiFiManagerParameter *custom_mqtt_port;
    WiFiManagerParameter *custom_mqtt_user;
    WiFiManagerParameter *custom_mqtt_password;

    static void saveConfigCallback();
};
