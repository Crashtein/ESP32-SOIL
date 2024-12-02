#include "Config.h"
#include "Debug.h"

void Config::load()
{
    if (!SPIFFS.begin(true))
    {
        outputDebugln("Failed to mount file system");
        return;
    }

    if (!SPIFFS.exists("/config.json"))
    {
        outputDebugln("Config file does not exist");
        return;
    }

    File configFile = SPIFFS.open("/config.json", "r");
    if (!configFile)
    {
        outputDebugln("Failed to open config file");
        return;
    }

    size_t size = configFile.size();
    if (size > 1024)
    {
        outputDebugln("Config file size is too large");
        return;
    }

    std::unique_ptr<char[]> buf(new char[size]);
    configFile.readBytes(buf.get(), size);

    StaticJsonDocument<256> json;
    DeserializationError error = deserializeJson(json, buf.get());
    if (error)
    {
        outputDebugln("Failed to parse config file");
        return;
    }

    strcpy(mqtt_server, json["mqtt_server"]);
    strcpy(mqtt_port, json["mqtt_port"]);
    strcpy(mqtt_user, json["mqtt_user"]);
    strcpy(mqtt_password, json["mqtt_password"]);

    outputDebugln("Loaded config: ");
    if (DEBUG == 1)
        serializeJsonPretty(json, Serial);
}

void Config::save()
{
    outputDebugln("Saving configuration to JSON");

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile)
    {
        outputDebugln("Failed to open config file for writing");
        return;
    }

    StaticJsonDocument<256> json;
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_user"] = mqtt_user;
    json["mqtt_password"] = mqtt_password;

    if (DEBUG == 1)
        serializeJsonPretty(json, Serial);
    serializeJson(json, configFile);
    configFile.close();
}

void Config::reset()
{
    SPIFFS.remove("/config.json");
}
