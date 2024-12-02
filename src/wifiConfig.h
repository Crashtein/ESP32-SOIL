#pragma once

#include <WiFiManager.h>
#include "Config.h"

class WifiConfig
{
public:
    static WifiConfig &getInstance();
    void setup();
    void startPortal();
    void reset();

private:
    WifiConfig();
    WiFiManager wifiManager;
    WiFiManagerParameter *custom_mqtt_server;
    WiFiManagerParameter *custom_mqtt_port;
    WiFiManagerParameter *custom_mqtt_user;
    WiFiManagerParameter *custom_mqtt_password;

    static void saveConfigCallback();
};
