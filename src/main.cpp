#include <Arduino.h>
#include "Config.h"
#include "MQTTManager.h"
#include "wifiConfig.h"
#include "Debug.h"
#include "OTAUpdater.h"

#include <TFT_eSPI.h> // Biblioteka TFT_eSPI
#include <SPI.h>
// Utwórz obiekt wyświetlacza
TFT_eSPI tft = TFT_eSPI();

const int potPin = 33;

// void updateProgress(int progress, int total) {
//     Serial.printf("Progress: %d%%\n", (progress / (total / 100)));
// }
// Pin przycisku do wybudzania - dostosuj do swojej konfiguracji
const int BUTTON_PIN = 0;    // Button 0 na T-Display
const int WAKE_TIME = 600000; // 600 sekund w milisekundach
unsigned long lastActivityTime = 0;

void goToSleep()
{
  Serial.println("Przechodzę w stan uśpienia...");
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(4);
  tft.setCursor(0,50);
  tft.println("SLEEP MODE");
  delay(1000); // Daj czas na wysłanie komunikatu przez Serial
  // Wyłączanie wyświetlacza (jeśli sterowany osobnym pinem)
  pinMode(4, OUTPUT); // Dostosuj pin do swojej płytki
  digitalWrite(4, LOW);
  // Konfiguracja pinu przycisku jako źródła wybudzenia
  esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, LOW); // LOW - wybudzenie przy niskim stanie

  // Przejście w deep sleep
  esp_deep_sleep_start();
}

void tftSetup(){
  // Inicjalizacja wyświetlacza
  tft.init();
  tft.setRotation(3); // Ustaw orientację: 1 = poziomo
  // Wypełnij ekran kolorem czarnym
  tft.fillScreen(TFT_BLACK);

  // Ustaw kolor tekstu i tło
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1); // Ustaw rozmiar tekstu
}


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
    outputDebug("Got input: ");
    outputDebugln(command);
    if (command.equals("reset"))
    {
      WifiConfig::getInstance().reset();
    }
    else if (command.equals("reconfigure"))
    {
      WifiConfig::getInstance().startPortal();
    }else if ("update"){
      update();
    }else{
      outputDebugln("Undefined command");
    }
  }
}

void setup() {
    Serial.begin(115200);
    OTAUpdater::getInstance().printInfo();
    tftSetup();
    
    tft.setTextSize(2);
    tft.setCursor(0, 8);
    tft.printf("Version: %s\n", PROJECT_VERSION);
    tft.setCursor(0, 40);
    tft.printf("Compilation date:\n%s\n%s\n", __DATE__, __TIME__);

    pinMode(36, INPUT);
    pinMode(37, INPUT);
    pinMode(38, INPUT);
    pinMode(39, INPUT);
    pinMode(32, INPUT);
    pinMode(33, INPUT);

    // Konfiguracja pinu przycisku
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // Sprawdź czy wybudzenie nastąpiło z deep sleep
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0)
    {
      Serial.println("Wybudzono przyciskiem");
    }

    // Zainicjuj czas ostatniej aktywności
    lastActivityTime = millis();

    Config::getInstance().load();
    WifiConfig::getInstance().setup();
    MQTTManager::getInstance().setup();

    tft.fillScreen(TFT_BLACK);
}

void loop() {
  checkSerialCommands();

  MQTTManager::getInstance().loop();
  Serial.printf("Read value for pin %d: %d\n", potPin, analogRead(potPin));
  tft.setTextSize(1);
  tft.setCursor(190, 0);
  tft.printf("B:%3.2fV",(1.19*2.0*analogRead(32)*3.3/4095.0));
  tft.setCursor(0, 8);
  tft.printf("Value for pin %d: %d   \n", 36, analogRead(36));
  tft.setCursor(0, 16);
  tft.printf("Value for pin %d: %d   \n", 37, analogRead(37));
  tft.setCursor(0, 24);
  tft.printf("Value for pin %d: %d   \n", 38, analogRead(38));
  tft.setCursor(0, 32);
  tft.printf("Value for pin %d: %d   \n", 39, analogRead(39));
  tft.setCursor(0, 40);
  // tft.printf("Value for pin %d: %d   \n", 32, analogRead(32));
  tft.setCursor(0, 48);
  tft.printf("Value for pin %d: %d   \n", 33, analogRead(33));

  // Sprawdź czy przycisk został naciśnięty
  if (digitalRead(BUTTON_PIN) == LOW)
  {
    Serial.println("Przycisk naciśnięty - resetuję timer");
    lastActivityTime = millis();
  }

  // Sprawdź czy minęło 60 sekund od ostatniej aktywności
  if (millis() - lastActivityTime > WAKE_TIME)
  {
    goToSleep();
  }
    delay(200);
  }
