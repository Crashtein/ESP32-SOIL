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
#include "MenuManager.h"

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

// Callbacki testowe
void onOptionSelected()
{
  Serial.println("Option selected!");
}

void onNavigate(const int currentIndex, const std::vector<String> currentMenuNames, const std::vector<bool> checkedVector)
{
  Serial.printf("Navigated to: %s\n", currentMenuNames[currentIndex].c_str());
}
MenuManager *menu = nullptr;
void buildMenu()
{
  menu = &MenuManager::getInstance();

  auto tft_showMenu = [](const int currentIndex, const std::vector<String> currentMenuNames, const std::vector<bool> checkedVector)
  { ExtendedTFT_eSPI::getInstance().showMenuOption(currentIndex, currentMenuNames, checkedVector); };

  // Tworzenie menu
  auto sensorsMenu = std::make_shared<MenuItem>("Sensors", onOptionSelected, onNavigate);
  auto optionsMenu = std::make_shared<MenuItem>("Menu", onOptionSelected, onNavigate);

  auto sensor1SubMenu = std::make_shared<MenuItem>("Sensor 1", onOptionSelected, onNavigate);
  auto sensor2SubMenu = std::make_shared<MenuItem>("Sensor 2", onOptionSelected, onNavigate);
  auto sensor3SubMenu = std::make_shared<MenuItem>("Sensor 3", onOptionSelected, onNavigate);
  auto sensor4SubMenu = std::make_shared<MenuItem>("Sensor 4", onOptionSelected, onNavigate);

  auto reconfigureSubMenu = std::make_shared<MenuItem>("Reconfigure", []()
                                                       { WifiConfig::getInstance().startPortal(); }, tft_showMenu);
  auto updateSubMenu = std::make_shared<MenuItem>("Update", []()
                                                  { OTAUpdater::getInstance().beginUpdate(); }, tft_showMenu);
  auto rfidSubMenu = std::make_shared<MenuItem>("RFID tools>>", []()
                                                { MenuManager::getInstance().navigateInto(); }, tft_showMenu);
  auto displayRFIDTagDataSubMenu = std::make_shared<MenuItem>("Read tag data", onOptionSelected, tft_showMenu,true);
  auto configureNewTagSubMenu = std::make_shared<MenuItem>("Configure new tag", onOptionSelected, tft_showMenu);
  auto copyRFIDTagDataSubMenu = std::make_shared<MenuItem>("Copy tag data", onOptionSelected, tft_showMenu);
  auto copyRFIDTagIDSubMenu = std::make_shared<MenuItem>("Copy tag ID", onOptionSelected, tft_showMenu);
  auto cloneRFIDTagSubMenu = std::make_shared<MenuItem>("Clone tag", onOptionSelected, tft_showMenu);
  auto resetAllConfigSubMenu = std::make_shared<MenuItem>("Reset ALL settings", []()
                                                          { WifiConfig::getInstance().reset(); }, tft_showMenu);

  // Tworzenie podmenu
  sensorsMenu->subMenu.push_back(sensor1SubMenu);
  sensorsMenu->subMenu.push_back(sensor2SubMenu);
  sensorsMenu->subMenu.push_back(sensor3SubMenu);
  sensorsMenu->subMenu.push_back(sensor4SubMenu);

  optionsMenu->subMenu.push_back(reconfigureSubMenu);
  optionsMenu->subMenu.push_back(updateSubMenu);
  rfidSubMenu->subMenu.push_back(displayRFIDTagDataSubMenu);
  rfidSubMenu->subMenu.push_back(configureNewTagSubMenu);
  rfidSubMenu->subMenu.push_back(copyRFIDTagDataSubMenu);
  rfidSubMenu->subMenu.push_back(copyRFIDTagIDSubMenu);
  rfidSubMenu->subMenu.push_back(cloneRFIDTagSubMenu);
  optionsMenu->subMenu.push_back(rfidSubMenu);
  optionsMenu->subMenu.push_back(resetAllConfigSubMenu);

  menu->setMainMenu({sensorsMenu, optionsMenu});
  #if DEBUG!=0
  menu->printMenu();
  #endif
  menu->navigateInto();
}

void navigationOnMenu()
{
  auto *pinManager = &PinManager::getInstance();
  if (pinManager->readDigitalPin(JOYSTICK_PIN_CENTER) && pinManager->digitalStateChanged(JOYSTICK_PIN_CENTER))
  {
    menu->selectCurrent();
  }
  if (pinManager->readDigitalPin(JOYSTICK_PIN_DOWN) && pinManager->digitalStateChanged(JOYSTICK_PIN_DOWN))
  {
    menu->navigateNext();
  }
  if (pinManager->readDigitalPin(JOYSTICK_PIN_UP) && pinManager->digitalStateChanged(JOYSTICK_PIN_UP))
  {
    menu->navigatePrevious();
  }
  if (pinManager->readDigitalPin(JOYSTICK_PIN_RIGHT) && pinManager->digitalStateChanged(JOYSTICK_PIN_RIGHT))
  {
    menu->navigateInto();
  }
  if (pinManager->readDigitalPin(JOYSTICK_PIN_LEFT) && pinManager->digitalStateChanged(JOYSTICK_PIN_LEFT))
  {
    auto depth = menu->getDepth();
    if(depth>1){
      menu->navigateBack();
    }
  }
  if (!pinManager->readDigitalPin(BUTTON_DOWN) && pinManager->digitalStateChanged(BUTTON_DOWN))
  {
    while (menu->getDepth() > 0)
      menu->navigateBack();
    menu->navigateNext();
    menu->navigateInto();
  }
  if (!pinManager->readDigitalPin(BUTTON_UP) && pinManager->digitalStateChanged(BUTTON_UP))
  {
    while (menu->getDepth() > 0)
      menu->navigateBack();
    menu->navigatePrevious();
    menu->navigateInto();
  }
  return;
}

void setup()
{
  Serial.begin(115200);
  // init digital and analog pins
  PinManager::getInstance().begin({JOYSTICK_PIN_UP, JOYSTICK_PIN_DOWN, JOYSTICK_PIN_LEFT, JOYSTICK_PIN_RIGHT, JOYSTICK_PIN_CENTER, BUTTON_UP, BUTTON_DOWN}, {SENSOR_PIN1, SENSOR_PIN2, SENSOR_PIN3, SENSOR_PIN4, BATTERY_VOLTAGE_PIN});
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  pinMode(BUTTON_UP, INPUT_PULLUP);
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
  buildMenu();
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

  // #if DEBUG!=0
  // if(PinManager::getInstance().anyDigitalStateChanged()){
  //   PinManager::getInstance().debug();
  // }
  // #endif

  if(menu->getCurrentItemMainMenuIndex()==0){
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
  }

  navigationOnMenu();
  delay(200);
}
