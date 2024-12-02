#include <Arduino.h>
#include "Config.h"
#include "MQTTManager.h"
#include "WifiConfig.h"
#include "Debug.h"
#include "OTAUpdater.h"

// void updateProgress(int progress, int total) {
//     Serial.printf("Progress: %d%%\n", (progress / (total / 100)));
// }

void update() {
    OTAUpdater& updater = OTAUpdater::getInstance();
    
    // Ustawienie callbacku postępu (opcjonalne)
    // updater.onProgress(updateProgress);

    updater.beginUpdate();
}


void checkSerialCommands() {
  if (Serial.available() > 0)
  {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.equals("reset"))
    {
      WifiConfig::getInstance().reset();
    }
    else if (command.equals("reconfigure"))
    {
      WifiConfig::getInstance().startPortal();
    }else if ("update"){
      update();
    }
  }
}

void setup() {
    Serial.begin(115200);
    OTAUpdater::getInstance().printInfo();
    
    Config::getInstance().load();
    WifiConfig::getInstance().setup();
    MQTTManager::getInstance().setup();
}

void loop() {
  checkSerialCommands();

  MQTTManager::getInstance().loop();
  delay(1000);
}
