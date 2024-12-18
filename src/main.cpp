#include <Arduino.h>
#include "Config.h"
#include "MQTTManager.h"
#include "wifiConfig.h"
#include "Debug.h"
#include "OTAUpdater.h"
#include "DHT22Reader.h"
#include "PinManager.h"

#include <TFT_eSPI.h> // Biblioteka TFT_eSPI
#include <SPI.h>
// Utwórz obiekt wyświetlacza
TFT_eSPI tft = TFT_eSPI();
const int WAKE_TIME = 600000; // 600 sekund w milisekundach
unsigned long lastActivityTime = 0;

void updateProgress(int current, int total) {
  // Pobieranie rozdzielczości wyświetlacza (wyświetlacz poziomo, zamienione parametry)
  int screenWidth = TFT_HEIGHT;
  int screenHeight = TFT_WIDTH;

  // Wymiary paska postępu jako procent rozdzielczości
  int barWidth = screenWidth * 0.8;        // Szerokość paska to 80% szerokości ekranu
  int barHeight = screenHeight * 0.05;     // Wysokość paska to 5% wysokości ekranu
  int barX = (screenWidth - barWidth) / 2; // Centrowanie paska na ekranie (X)
  int barY = screenHeight / 2 - barHeight; // Umieszczanie paska mniej więcej w połowie ekranu (Y)

  // Obliczanie procentowego postępu
  float progress = (float)current / total;
  int filledWidth = (int)(barWidth * progress); // Szerokość wypełnionej części paska
  int percent = (int)(progress * 100);          // Obliczenie procentów

  // Rysowanie tła paska postępu
  tft.fillRect(barX, barY, barWidth, barHeight, TFT_DARKGREY);

  // Rysowanie wypełnionej części paska postępu
  tft.fillRect(barX, barY, filledWidth, barHeight, TFT_GREEN);

  // Rysowanie ramki wokół paska
  tft.drawRect(barX, barY, barWidth, barHeight, TFT_WHITE);

  // Wyświetlanie tekstu z procentami
  char buffer[20];
  snprintf(buffer, sizeof(buffer), "Updating %d%%", percent);

  // Pozycja tekstu poniżej paska postępu
  int textX = (screenWidth - tft.textWidth(buffer)) / 2; // Centrowanie tekstu
  int textY = barY + barHeight + 10;                     // 10 pikseli poniżej paska

  // Wyczyść poprzedni tekst
  tft.fillRect(0, textY - 5, screenWidth, tft.fontHeight() + 10, TFT_BLACK);

  // Wyświetl tekst
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(buffer, textX, textY);
}


void goToSleep()
{
  Serial.println("Przechodzę w stan uśpienia...");
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(4);
  tft.setCursor(0, 50);
  tft.println("SLEEP MODE");
  delay(1000); // Daj czas na wysłanie komunikatu przez Serial
  // Wyłączanie wyświetlacza (jeśli sterowany osobnym pinem)
  pinMode(4, OUTPUT); // Dostosuj pin do swojej płytki
  digitalWrite(4, LOW);
  // Konfiguracja pinu przycisku jako źródła wybudzenia
  esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_UP, LOW); // LOW - wybudzenie przy niskim stanie
  // Przejście w deep sleep
  esp_deep_sleep_start();
}

void tftSetup()
{
  // Inicjalizacja wyświetlacza
  tft.init();
  tft.setRotation(1); // Ustaw orientację: 1 = poziomo
  // Wypełnij ekran kolorem czarnym
  tft.fillScreen(TFT_BLACK);

  // Ustaw kolor tekstu i tło
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1); // Ustaw rozmiar tekstu
}

void update()
{
  OTAUpdater &updater = OTAUpdater::getInstance();
  tft.fillScreen(TFT_BLACK);
  // Ustawienie callbacku postępu (opcjonalne)
  updater.onProgress(updateProgress);

  updater.beginUpdate();
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
  PinManager::getInstance().begin({JOYSTICK_PIN_UP, JOYSTICK_PIN_DOWN, JOYSTICK_PIN_LEFT, JOYSTICK_PIN_RIGHT, JOYSTICK_PIN_CENTER, BUTTON_UP, BUTTON_DOWN}, {SENSOR_PIN1, SENSOR_PIN2, SENSOR_PIN3, SENSOR_PIN4, BATTERY_VOLTAGE_PIN});
  OTAUpdater::getInstance().printInfo();
  Config::getInstance().load();

  tftSetup();
  tft.setTextSize(2);
  tft.setCursor(0, 8);
  tft.printf("Version:\n%s\n", PROJECT_VERSION);
  tft.setCursor(0, 40);
  tft.printf("Compilation date:\n%s\n%s\n", __DATE__, __TIME__);

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

  tft.fillScreen(TFT_BLACK);

  // Sprawdź czy wybudzenie nastąpiło z deep sleep
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0)
  {
    Serial.println("Wybudzono przyciskiem");
  }
  // Zainicjuj czas ostatniej aktywności
  lastActivityTime = millis();
}

void updateStatusBar()
{
  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(0, 0);
  tft.printf("H:%3.1f%% T:%3.1f÷C", DHT22Reader::getInstance()->getHumidity(), DHT22Reader::getInstance()->getTemperature());
  tft.setCursor(145, 0);

  if (WifiConfig::getInstance().getStatus())
  {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
  }
  else
  {
    tft.setTextColor(TFT_RED, TFT_BLACK);
  }
  tft.print("WiFi");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print("|");
  if (MQTTManager::getInstance().getStatus())
  {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
  }
  else
  {
    tft.setTextColor(TFT_RED, TFT_BLACK);
  }
  tft.print("MQTT");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print("|");
  tft.setCursor(206, 0);
  float batteryVoltage = 1.122 * 2.0 * analogRead(BATTERY_VOLTAGE_PIN) * 3.3 / 4095.0;
  if (batteryVoltage > 4.2)
  {
    tft.setTextColor(TFT_BLUE, TFT_BLACK);
  }
  else if (batteryVoltage > 3.7)
  {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
  }
  else if (batteryVoltage > 3.4)
  {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  }
  else
  {
    tft.setTextColor(TFT_RED, TFT_BLACK);
  }
  tft.printf("%3.2fV", batteryVoltage);
}

void loop()
{
  checkSerialCommands();
  MQTTManager::getInstance().loop();
  PinManager::getInstance().update();

  if(DEBUG){
    PinManager::getInstance().debug();
  }

  updateStatusBar();
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 8);
  tft.printf("Sensor #1: %d   \n", analogRead(SENSOR_PIN1));
  tft.setCursor(0, 24);
  tft.printf("Sensor #2: %d   \n", analogRead(SENSOR_PIN2));
  tft.setCursor(0, 40);
  tft.printf("Sensor #3: %d   \n", analogRead(SENSOR_PIN3));
  tft.setCursor(0, 56);
  tft.printf("Sensor #4: %d   \n", analogRead(SENSOR_PIN4));

  // Sprawdź czy przycisk został naciśnięty
  if (PinManager::getInstance().anyDigitalStateChanged())
  {
    outputDebugln("Button pressed, resetting sleep timer");
    lastActivityTime = millis();
  }

  // Sprawdź czy minęło 60 sekund od ostatniej aktywności
  if (millis() - lastActivityTime > WAKE_TIME)
  {
    goToSleep();
  }
  delay(200);
}
