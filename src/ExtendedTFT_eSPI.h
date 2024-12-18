#ifndef EXTENDEDTFT_ESPI_H
#define EXTENDEDTFT_ESPI_H

#include <TFT_eSPI.h> // Biblioteka TFT_eSPI
#include "Debug.h"

class ExtendedTFT_eSPI : public TFT_eSPI
{
public:
    // Statyczna metoda dostępu do instancji singletonu
    static ExtendedTFT_eSPI &getInstance();

    void init();

    void setRotation(int rotation=0);

    // Metody związane ze spritem
    void startDrawingToSprite();
    void pushSpriteToScreen(int x=0, int y=0);
    void endSprite();

    // Przykład metody do rysowania do sprita
    void drawTextInSprite(const char *text, int x, int y, uint8_t fontSize);

    // Usunięcie kopiowania i przenoszenia
    ExtendedTFT_eSPI(const ExtendedTFT_eSPI &) = delete;
    ExtendedTFT_eSPI &operator=(const ExtendedTFT_eSPI &) = delete;

    //funkcja do callbacku OTA update
    void updateOTAProgress(int current, int total);

    //zwraca szerokość uwzględniając orientację
    int owidth() { return (rotation % 2 == 1) ? width() : height(); }
    //zwraca wysokość uwzględniając orientację
    int oheight() { return (rotation % 2 == 1) ? height() : width(); }

private:
    int oldCurrentOTAProgress = -1;
    int tftWidth;
    int tftHeight;

    TFT_eSprite *sprite; // Pole do obsługi sprita
    // Prywatny konstruktor i destruktor
    ExtendedTFT_eSPI();
    ~ExtendedTFT_eSPI();
};

#endif // EXTENDEDTFT_ESPI_H
