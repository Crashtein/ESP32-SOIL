#pragma once

#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "wifiConfig.h"

class Config
{
public:
    char mqtt_server[40];
    char mqtt_port[6];
    char mqtt_user[40];
    char mqtt_password[40];

    void load();
    void save();
    void reset();
    char *getUrl() { return mqtt_server + 6; }

    static Config &getInstance()
    {
        static Config instance;
        return instance;
    }

private:
    Config() {}
};
