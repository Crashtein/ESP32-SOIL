#include "WifiConfig.h"
#include "Debug.h"

static char passwordNoChangeMask[] = "If provided earlier leave it to use saved";

WifiConfig& WifiConfig::getInstance() {
    static WifiConfig instance;
    return instance;
}

WifiConfig::WifiConfig() {
    Config& config = Config::getInstance();
    
    custom_mqtt_server = new WiFiManagerParameter("server", "MQTT server (ssl://<domain> or tcp://<domain>)", 
        DEFAULT_MQTT_SERVER, 40);
    custom_mqtt_port = new WiFiManagerParameter("port", "MQTT port", 
        DEFAULT_MQTT_PORT, 6);
    custom_mqtt_user = new WiFiManagerParameter("user", "MQTT user", 
        config.mqtt_user, 40);
    custom_mqtt_password = new WiFiManagerParameter("password", "MQTT password", 
        passwordNoChangeMask, 40);
}

void WifiConfig::setup() {
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.addParameter(custom_mqtt_server);
    wifiManager.addParameter(custom_mqtt_port);
    wifiManager.addParameter(custom_mqtt_user);
    wifiManager.addParameter(custom_mqtt_password);

    if (!wifiManager.autoConnect(AP_SSID)) {
        outputDebugln("Failed to connect and hit timeout");
        delay(3000);
        ESP.restart();
    }
    
}

void WifiConfig::saveConfigCallback() {
    Config& config = Config::getInstance();
    WifiConfig& wifiConfig = WifiConfig::getInstance();
    
    strcpy(config.mqtt_server, wifiConfig.custom_mqtt_server->getValue());
    strcpy(config.mqtt_port, wifiConfig.custom_mqtt_port->getValue());
    strcpy(config.mqtt_user, wifiConfig.custom_mqtt_user->getValue());
    
    if (strcmp(wifiConfig.custom_mqtt_password->getValue(), passwordNoChangeMask) != 0) {
        outputDebugln("MQTT password different than masked template");
        strcpy(config.mqtt_password, wifiConfig.custom_mqtt_password->getValue());
        wifiConfig.custom_mqtt_password->setValue(passwordNoChangeMask, 40);
    } else {
        outputDebugln("Leaving MQTT password unchanged");
    }
    
    config.save();
}

void WifiConfig::startPortal() {
    wifiManager.startConfigPortal(AP_SSID);
    outputDebugln("Reconfiguration completed restarting");
    ESP.restart();
}

void WifiConfig::reset() {
    wifiManager.resetSettings();
    Config::getInstance().reset();
    ESP.restart();
}
