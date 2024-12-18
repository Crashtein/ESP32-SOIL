#include "MQTTManager.h"
#include "Debug.h"

MQTTManager &MQTTManager::getInstance()
{
    static MQTTManager instance;
    return instance;
}

void MQTTManager::setup()
{
    Config &config = Config::getInstance();

    if (strncmp(config.mqtt_server, "ssl://", 6) == 0)
    {
        setupSSL();
    }
    else if (strncmp(config.mqtt_server, "tcp://", 6) == 0)
    {
        setupTCP();
    }
    else
    {
        outputDebugln("Wrong MQTT protocol");
        return;
    }

    outputDebug("Connecting to MQTT URL: ");
    outputDebugln(config.getUrl());
    client->setServer(config.getUrl(), atoi(config.mqtt_port));
}

void MQTTManager::setupSSL()
{
    outputDebugln("Setting up SSL client");
    espClientSecure.setCACert(Certificates::ROOT_CA);
    client = new PubSubClient(espClientSecure);
}

void MQTTManager::setupTCP()
{
    outputDebugln("Setting up no SSL client");
    client = new PubSubClient(espClient);
}

void MQTTManager::reconnect()
{
    Config &config = Config::getInstance();

    if (client->connect("ESP32-SOIL", config.mqtt_user, config.mqtt_password))
    {
        outputDebugln("MQTT connected");
    }
    else
    {
        outputDebug("MQTT failed, rc=");
        outputDebug(client->state());
        outputDebugln(" try again in 5 seconds");
        // delay(5000);
    }
}

bool MQTTManager::getStatus()
{
    return (client->connected());
}

void MQTTManager::loop()
{
    if (!client->connected())
    {
        reconnect();
    }
    client->loop();
}
