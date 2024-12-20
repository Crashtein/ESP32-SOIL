#include <Arduino.h>
#include "Config.h"
#include "MQTTManager.h"
#include "wifiConfig.h"
#include "Debug.h"
#include "OTAUpdater.h"
#include "DHT22Reader.h"
#include "PinManager.h"
#include "ExtendedTFT_eSPI.h"
#include <SPI.h>

ExtendedTFT_eSPI *tft = nullptr;

void goToSleep()
{
  Serial.println("Przechodzę w stan uśpienia...");
  tft->printSleepInfo();
  delay(1000);
  // Wyłączanie wyświetlacza
  tft->turnOff();
  // Konfiguracja pinu przycisku jako źródła wybudzenia
  esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_UP, LOW); // LOW - wybudzenie przy niskim stanie
  // Przejście w deep sleep
  esp_deep_sleep_start();
}

void checkSerialCommands()
{
  if (Serial.available() > 0)
  {
    String command = Serial.readStringUntil('\n');
    command.trim();
    outputDebug("Got input: ");
    outputDebugln(command);
    if (command.equals("reset"))
    {
      WifiConfig::getInstance().reset();
    }
    else if (command.equals("reconfigure"))
    {
      WifiConfig::getInstance().startPortal();
    }
    else if (command.equals("update"))
    {
      OTAUpdater::getInstance().beginUpdate();
    }
    else
    {
      Serial.println("Undefined command");
    }
  }
}

void setup()
{
  Serial.begin(115200);
  // init digital and analog pins
  PinManager::getInstance().begin({JOYSTICK_PIN_UP, JOYSTICK_PIN_DOWN, JOYSTICK_PIN_LEFT, JOYSTICK_PIN_RIGHT, JOYSTICK_PIN_CENTER, BUTTON_UP, BUTTON_DOWN}, {SENSOR_PIN1, SENSOR_PIN2, SENSOR_PIN3, SENSOR_PIN4, BATTERY_VOLTAGE_PIN});
  OTAUpdater::getInstance().printInfo();
  Config::getInstance().load();
  tft = &ExtendedTFT_eSPI::getInstance();
  tft->initDefault();

  tft->printProgramInfo();

  WifiConfig::getInstance().setAPCallback([](WiFiManager *wm)
                                          { ExtendedTFT_eSPI::getInstance().wifiAPcallback(wm); });
  if (digitalRead(BUTTON_UP) == LOW && digitalRead(JOYSTICK_PIN_DOWN) == HIGH)
  {
    WifiConfig::getInstance().startPortal();
  }

  WifiConfig::getInstance().setup();

  OTAUpdater::getInstance().onProgress([](int current, int total)
                                       { ExtendedTFT_eSPI::getInstance().updateOTAProgressCallback(current, total); });
  if (digitalRead(BUTTON_UP) == LOW && digitalRead(JOYSTICK_PIN_UP) == HIGH)
  {
    OTAUpdater::getInstance().beginUpdate();
  }
  MQTTManager::getInstance().setup();

  // Sprawdź czy wybudzenie nastąpiło z deep sleep
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0)
  {
    Serial.println("Wybudzono przyciskiem");
  }
  tft->clear();
}

void loop()
{
  checkSerialCommands();
  MQTTManager::getInstance().loop();
  PinManager::getInstance().update();

  // Sprawdź czy miną zdefiniowany czas od ostatniej aktywności
  if (millis() - PinManager::getInstance().getLastActivityTime() > SLEEP_TIME)
    goToSleep();

  // if(DEBUG){
  //   PinManager::getInstance().debug();
  // }

  tft->drawStatusBar();

  tft->setTextColor(TFT_WHITE, TFT_BLACK);
  tft->setTextSize(2);
  tft->setCursor(0, 8);
  tft->printf("Sensor #1: %d   \n", analogRead(SENSOR_PIN1));
  tft->setCursor(0, 24);
  tft->printf("Sensor #2: %d   \n", analogRead(SENSOR_PIN2));
  tft->setCursor(0, 40);
  tft->printf("Sensor #3: %d   \n", analogRead(SENSOR_PIN3));
  tft->setCursor(0, 56);
  tft->printf("Sensor #4: %d   \n", analogRead(SENSOR_PIN4));

  delay(200);
}
