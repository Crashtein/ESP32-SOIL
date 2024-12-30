#ifndef EXTENDEDTFT_ESPI_H
#define EXTENDEDTFT_ESPI_H

#include <TFT_eSPI.h> // Biblioteka TFT_eSPI
#include "Debug.h"
#include "DHT22Reader.h"
#include "wifiConfig.h"
#include "MQTTManager.h"
#include "WiFiManager.h"

class ExtendedTFT_eSPI : public TFT_eSPI
{
public:
    // Statyczna metoda dostępu do instancji singletonu
    static ExtendedTFT_eSPI &getInstance();

    void init();

    void initDefault();

    void setRotation(int rotation = 0);

    // Metody związane ze spritem
    void startDrawingToSprite();
    void pushSpriteToScreen(int x = 0, int y = 0);
    void endSprite();

    // Przykład metody do rysowania do sprita
    void drawTextInSprite(const char *text, int x, int y, uint8_t fontSize);

    // Usunięcie kopiowania i przenoszenia
    ExtendedTFT_eSPI(const ExtendedTFT_eSPI &) = delete;
    ExtendedTFT_eSPI &operator=(const ExtendedTFT_eSPI &) = delete;

    // funkcja do callbacku OTA update
    void updateOTAProgressCallback(int current, int total);

    // wyświetla na ekranie wersję i datę kompilacji
    void printVersionInfo();

    // funkcja do rysowania status baru
    void drawStatusBar();

    // callback dla ustawionego AP przez WIFI
    void wifiAPcallback(WiFiManager *wm);

    // wyczyść ekran
    void clear() { fillScreen(TFT_BLACK); }

    // wyświetl informacje o wersji programu
    void printProgramInfo();

    // wyświetl informacje przed przejściem w stan uśpienia
    void printSleepInfo();

    // wyłącza wyświetlacz
    void turnOff() { digitalWrite(TFT_BL, LOW); }

    // show menu option
    void showMenuOption(const int currentIndex, std::vector<String>, const std::vector<bool> checkedVector);

private:
    int oldCurrentOTAProgress = -1;
    float oldBatteryVoltage=4;

    TFT_eSprite *sprite; // Pole do obsługi sprita
    // Prywatny konstruktor i destruktor
    ExtendedTFT_eSPI();
    ~ExtendedTFT_eSPI();
};

#endif // EXTENDEDTFT_ESPI_H
