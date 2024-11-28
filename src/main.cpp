#include <Arduino.h>
#include <WiFiManager.h>
#include <PubSubClient.h>

WiFiManager wifiManager;
WiFiClient espClient;
PubSubClient client(espClient);

char mqtt_server[40];
char mqtt_port[6];
char mqtt_user[40];
char mqtt_password[40];

WiFiManagerParameter custom_mqtt_server("server", "MQTT server", mqtt_server, 40);
WiFiManagerParameter custom_mqtt_port("port", "MQTT port", mqtt_port, 6);
WiFiManagerParameter custom_mqtt_user("user", "MQTT user", mqtt_user, 40);
WiFiManagerParameter custom_mqtt_password("password", "MQTT password", mqtt_password, 40);

void saveConfigCallback()
{
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_user, custom_mqtt_user.getValue());
  strcpy(mqtt_password, custom_mqtt_password.getValue());
}

void setup()
{
  Serial.begin(115200);

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

  client.setServer(mqtt_server, atoi(mqtt_port));
}

void reconnect()
{
  if (client.connect("ESP32-SOIL", mqtt_user, mqtt_password))
  {
    Serial.println("MQTT connected");
  }
  else
  {
    Serial.print("MQTT failed, rc=");
    Serial.print(client.state());
    Serial.println(" try again in 5 seconds");
    delay(5000);
  }
}

void resetConfig()
{
  wifiManager.resetSettings();
  ESP.restart();
}

void loop()
{
  if (Serial.available() > 0)
  {
    String command = Serial.readStringUntil('\n');
    Serial.println("Got command: "+ command);
    command.trim();
    if (command.equals("reset"))
    {
      resetConfig();
    }
  }

  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  delay(1000);
}
