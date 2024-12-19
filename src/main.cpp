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
// ExtendedTFT_eSPI &tft = ExtendedTFT_eSPI::getInstance();
const int WAKE_TIME = 600000; // 600 sekund w milisekundach
unsigned long lastActivityTime = 0;

void goToSleep()
{
  Serial.println("Przechodzę w stan uśpienia...");
  tft->fillScreen(TFT_BLACK);
  tft->setTextSize(4);
  tft->setCursor(0, 50);
  tft->println("SLEEP MODE");
  delay(1000); // Daj czas na wysłanie komunikatu przez Serial
  // Wyłączanie wyświetlacza (jeśli sterowany osobnym pinem)
  pinMode(4, OUTPUT); // Dostosuj pin do swojej płytki
  digitalWrite(4, LOW);
  // Konfiguracja pinu przycisku jako źródła wybudzenia
  esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_UP, LOW); // LOW - wybudzenie przy niskim stanie
  // Przejście w deep sleep
  esp_deep_sleep_start();
}

void update()
{
  OTAUpdater::getInstance().onProgress([](int current, int total)
                                       { ExtendedTFT_eSPI::getInstance().updateOTAProgress(current, total); });
  OTAUpdater::getInstance().beginUpdate();
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
    else if ("update")
    {
      update();
    }
    else
    {
      outputDebugln("Undefined command");
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

  // tft->printVersionInfo();
  tft->setTextSize(2);
  tft->setCursor(0, 8);
  tft->printf("Version:\n%s\n", PROJECT_VERSION);
  tft->setCursor(0, 40);
  tft->printf("Compilation date:\n%s\n%s\n", __DATE__, __TIME__);

  if (digitalRead(BUTTON_UP) == LOW && digitalRead(JOYSTICK_PIN_DOWN) == HIGH)
  {
    WifiConfig::getInstance().startPortal();
  }

  WifiConfig::getInstance().setup();
  if (digitalRead(BUTTON_UP) == LOW && digitalRead(JOYSTICK_PIN_UP) == HIGH)
  {
    update();
  }
  MQTTManager::getInstance().setup();

  // Sprawdź czy wybudzenie nastąpiło z deep sleep
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0)
  {
    Serial.println("Wybudzono przyciskiem");
  }
  // Zainicjuj czas ostatniej aktywności
  lastActivityTime = millis();
  tft->fillScreen(TFT_BLACK);
}

void updateStatusBar()
{
  tft->setTextSize(1);
  tft->setTextColor(TFT_YELLOW, TFT_BLACK);
  tft->setCursor(0, 0);
  tft->printf("H:%3.1f%% T:%3.1f÷C", DHT22Reader::getInstance()->getHumidity(), DHT22Reader::getInstance()->getTemperature());
  tft->setCursor(145, 0);

  if (WifiConfig::getInstance().getStatus())
  {
    tft->setTextColor(TFT_GREEN, TFT_BLACK);
  }
  else
  {
    tft->setTextColor(TFT_RED, TFT_BLACK);
  }
  tft->print("WiFi");
  tft->setTextColor(TFT_WHITE, TFT_BLACK);
  tft->print("|");
  if (MQTTManager::getInstance().getStatus())
  {
    tft->setTextColor(TFT_GREEN, TFT_BLACK);
  }
  else
  {
    tft->setTextColor(TFT_RED, TFT_BLACK);
  }
  tft->print("MQTT");
  tft->setTextColor(TFT_WHITE, TFT_BLACK);
  tft->print("|");
  tft->setCursor(206, 0);
  float batteryVoltage = 1.122 * 2.0 * analogRead(BATTERY_VOLTAGE_PIN) * 3.3 / 4095.0;
  if (batteryVoltage > 4.2)
  {
    tft->setTextColor(TFT_BLUE, TFT_BLACK);
  }
  else if (batteryVoltage > 3.7)
  {
    tft->setTextColor(TFT_GREEN, TFT_BLACK);
  }
  else if (batteryVoltage > 3.4)
  {
    tft->setTextColor(TFT_YELLOW, TFT_BLACK);
  }
  else
  {
    tft->setTextColor(TFT_RED, TFT_BLACK);
  }
  tft->printf("%3.2fV", batteryVoltage);
}

void loop()
{
  checkSerialCommands();
  // MQTTManager::getInstance().loop();
  PinManager::getInstance().update();

  // if(DEBUG){
  //   PinManager::getInstance().debug();
  // }

  // tft->startDrawingToSprite();
  // tft->drawStatusBar();
  // tft->pushSpriteToScreen(0,0);
  updateStatusBar();
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

  // Sprawdź czy jakiś przycisk zmienił stan
  if (PinManager::getInstance().anyDigitalStateChanged())
  {
    outputDebugln("Some buttons changed its state, resetting sleep timer");
    lastActivityTime = millis();
  }

  // Sprawdź czy minęło 60 sekund od ostatniej aktywności
  if (millis() - lastActivityTime > WAKE_TIME)
  {
    goToSleep();
  }
  delay(200);
}
