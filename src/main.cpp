#define DEBUG 1

#if DEBUG == 1
#define outputDebug(x); Serial.print(x);
#define outputDebugln(x); Serial.println(x);
#else
#define outputDebug(x);
#define outputDebugln(x);
#endif

#include <Arduino.h>
#include <WiFiManager.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <PubSubclient.h>
#include <WiFiClientSecure.h>
#include <FS.h>

WiFiManager wifiManager;
WiFiClientSecure espClientSecure;
WiFiClient espClient;
PubSubClient *client;

char mqtt_server[40];
char *url = mqtt_server + 6;
char mqtt_port[6];
char mqtt_user[40];
char mqtt_password[40];
static char passwordNoChangeMask[] = "Leave it to use saved";

const char *rootCA = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIEVzCCAj+gAwIBAgIRALBXPpFzlydw27SHyzpFKzgwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMjQwMzEzMDAwMDAw
WhcNMjcwMzEyMjM1OTU5WjAyMQswCQYDVQQGEwJVUzEWMBQGA1UEChMNTGV0J3Mg
RW5jcnlwdDELMAkGA1UEAxMCRTYwdjAQBgcqhkjOPQIBBgUrgQQAIgNiAATZ8Z5G
h/ghcWCoJuuj+rnq2h25EqfUJtlRFLFhfHWWvyILOR/VvtEKRqotPEoJhC6+QJVV
6RlAN2Z17TJOdwRJ+HB7wxjnzvdxEP6sdNgA1O1tHHMWMxCcOrLqbGL0vbijgfgw
gfUwDgYDVR0PAQH/BAQDAgGGMB0GA1UdJQQWMBQGCCsGAQUFBwMCBggrBgEFBQcD
ATASBgNVHRMBAf8ECDAGAQH/AgEAMB0GA1UdDgQWBBSTJ0aYA6lRaI6Y1sRCSNsj
v1iU0jAfBgNVHSMEGDAWgBR5tFnme7bl5AFzgAiIyBpY9umbbjAyBggrBgEFBQcB
AQQmMCQwIgYIKwYBBQUHMAKGFmh0dHA6Ly94MS5pLmxlbmNyLm9yZy8wEwYDVR0g
BAwwCjAIBgZngQwBAgEwJwYDVR0fBCAwHjAcoBqgGIYWaHR0cDovL3gxLmMubGVu
Y3Iub3JnLzANBgkqhkiG9w0BAQsFAAOCAgEAfYt7SiA1sgWGCIpunk46r4AExIRc
MxkKgUhNlrrv1B21hOaXN/5miE+LOTbrcmU/M9yvC6MVY730GNFoL8IhJ8j8vrOL
pMY22OP6baS1k9YMrtDTlwJHoGby04ThTUeBDksS9RiuHvicZqBedQdIF65pZuhp
eDcGBcLiYasQr/EO5gxxtLyTmgsHSOVSBcFOn9lgv7LECPq9i7mfH3mpxgrRKSxH
pOoZ0KXMcB+hHuvlklHntvcI0mMMQ0mhYj6qtMFStkF1RpCG3IPdIwpVCQqu8GV7
s8ubknRzs+3C/Bm19RFOoiPpDkwvyNfvmQ14XkyqqKK5oZ8zhD32kFRQkxa8uZSu
h4aTImFxknu39waBxIRXE4jKxlAmQc4QjFZoq1KmQqQg0J/1JF8RlFvJas1VcjLv
YlvUB2t6npO6oQjB3l+PNf0DpQH7iUx3Wz5AjQCi6L25FjyE06q6BZ/QlmtYdl/8
ZYao4SRqPEs/6cAiF+Qf5zg2UkaWtDphl1LKMuTNLotvsX99HP69V2faNyegodQ0
LyTApr/vT01YPE46vNsDLgK+4cL6TrzC/a4WcmF5SRJ938zrv/duJHLXQIku5v0+
EwOy59Hdm0PT/Er/84dDV0CSjdR/2XuZM3kpysSKLgD1cKiDA+IRguODCxfO9cyY
Ig46v9mFmBvyH04=
-----END CERTIFICATE-----
)EOF";

WiFiManagerParameter custom_mqtt_server("server", "MQTT server", "ssl://<domain> or tcp://<domain>", sizeof(mqtt_server));
WiFiManagerParameter custom_mqtt_port("port", "MQTT port", mqtt_port, sizeof(mqtt_port));
WiFiManagerParameter custom_mqtt_user("user", "MQTT user", mqtt_user, sizeof(mqtt_user));
WiFiManagerParameter custom_mqtt_password("password", "MQTT password", mqtt_password, sizeof(mqtt_password));

void saveConfigCallback()
{
  outputDebugln("Saving configuration to JSON");
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_user, custom_mqtt_user.getValue());
  if (strcmp(custom_mqtt_password.getValue(), passwordNoChangeMask) != 0)
  {
    outputDebugln("MQTT password different than masked template");
    strcpy(mqtt_password, custom_mqtt_password.getValue());
    custom_mqtt_password.setValue(passwordNoChangeMask, sizeof(mqtt_password));
  }else
  {
    outputDebugln("Leaving MQTT password unchanged");
  }
  
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile)
  {
    Serial.println("Failed to open config file for writing");
    return;
  }

  StaticJsonDocument<256> json;
  json["mqtt_server"] = mqtt_server;
  json["mqtt_port"] = mqtt_port;
  json["mqtt_user"] = mqtt_user;
  json["mqtt_password"] = mqtt_password;
  if(DEBUG == 1)
    serializeJsonPretty(json, Serial);
  serializeJson(json, configFile);
  configFile.close();
}

void loadConfig()
{
  if (!SPIFFS.begin(true))
  {
    Serial.println("Failed to mount file system");
    return;
  }

  if (!SPIFFS.exists("/config.json"))
  {
    Serial.println("Config file does not exist");
    return;
  }

  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile)
  {
    Serial.println("Failed to open config file");
    return;
  }

  size_t size = configFile.size();
  if (size > 1024)
  {
    Serial.println("Config file size is too large");
    return;
  }

  std::unique_ptr<char[]> buf(new char[size]);
  configFile.readBytes(buf.get(), size);

  StaticJsonDocument<256> json;
  DeserializationError error = deserializeJson(json, buf.get());
  if (error)
  {
    Serial.println("Failed to parse config file");
    return;
  }
  strcpy(mqtt_server, json["mqtt_server"]);
  strcpy(mqtt_port, json["mqtt_port"]);
  strcpy(mqtt_user, json["mqtt_user"]);
  strcpy(mqtt_password, json["mqtt_password"]);
  custom_mqtt_server.setValue(mqtt_server, sizeof(mqtt_server));
  custom_mqtt_port.setValue(mqtt_port, sizeof(mqtt_port));
  custom_mqtt_user.setValue(mqtt_user, sizeof(mqtt_user));
  custom_mqtt_password.setValue(passwordNoChangeMask, sizeof(mqtt_password));
  outputDebugln("Loadeded config: ");
  if (DEBUG == 1)
      serializeJsonPretty(json, Serial);
}

void reconfigure()
{
  wifiManager.startConfigPortal("ESP32-SOIL");
  outputDebugln("Reconfiguration completed restarting");
  ESP.restart();
}

void setup()
{
  Serial.begin(115200);
  loadConfig();

  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_password);

  if (!wifiManager.autoConnect("ESP32-SOIL"))
  {
    Serial.println("Failed to connect and hit timeout");
    delay(3000);
    ESP.restart();
  }
  if (strncmp(mqtt_server, "ssl://", 6) == 0)
  {
    outputDebugln("Setting up SSL client");
    espClientSecure.setCACert(rootCA);
    client = new PubSubClient(espClientSecure);
  }
  else if (strncmp(mqtt_server, "tcp://", 6) == 0)
  {
    outputDebugln("Setting up no SSL client");
    client = new PubSubClient(espClient);
  }
  else
  {
    Serial.println("Wrong MQTT protocol, can be ssl://<domain> or tcp://<domain>, running reconfiguration");
    reconfigure();
  }
  outputDebug("Connecting to MQTT URL: ");
  outputDebugln(url);
  client->setServer(url, atoi(mqtt_port));
}

void reconnect()
{
  if (client->connect("ESP32-SOIL", mqtt_user, mqtt_password))
  {
    Serial.println("MQTT connected");
  }
  else
  {
    Serial.print("MQTT failed, rc=");
    Serial.print(client->state());
    Serial.println(" try again in 5 seconds");
    delay(5000);
  }
}

void resetConfig()
{
  wifiManager.resetSettings();
  SPIFFS.remove("/config.json");
  ESP.restart();
}

void loop()
{
  if (Serial.available() > 0)
  {
    String command = Serial.readStringUntil('\n');
    Serial.println("Got command: " + command);
    command.trim();
    if (command.equals("reset"))
    {
      resetConfig();
    }
    else if (command.equals("reconfigure"))
    {
      reconfigure();
    }
    else
    {
      Serial.println("Command not recognized");
    }
  }

  if (!client->connected())
  {
    reconnect();
  }
  client->loop();

  delay(1000);
}
