#pragma once

#include <PubSubclient.h>
#include <WiFiClientSecure.h>
#include "Config.h"
#include "Certificates.h"

class MQTTManager
{
public:
    static MQTTManager &getInstance();
    void setup();
    void loop();
    PubSubClient *getClient() { return client; }

private:
    MQTTManager() {};
    void reconnect();
    void setupSSL();
    void setupTCP();

    PubSubClient *client;
    WiFiClientSecure espClientSecure;
    WiFiClient espClient;
    const char *rootCA; // Define your root CA certificate here
};
