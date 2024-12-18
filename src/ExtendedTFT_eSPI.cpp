#include "ExtendedTFT_eSPI.h"

ExtendedTFT_eSPI &ExtendedTFT_eSPI::getInstance()
{
    static ExtendedTFT_eSPI instance;
    return instance;
}

ExtendedTFT_eSPI::ExtendedTFT_eSPI() : TFT_eSPI(), sprite(nullptr) {

}

ExtendedTFT_eSPI::~ExtendedTFT_eSPI()
{
    if (sprite)
    {
        sprite->deleteSprite();
        delete sprite;
    }
}

void ExtendedTFT_eSPI::init()
{
    TFT_eSPI::init();
    fillScreen(TFT_BLACK);
}

void ExtendedTFT_eSPI::setRotation(int rotation)
{
    TFT_eSPI::setRotation(rotation);
    if (rotation % 2 == 1)  //ekrean poziomo, rotation = 1 lub 3
    {
        tftWidth = height();
        tftHeight = width();
    }
    else
    {
        tftWidth = width();
        tftHeight = height();
    }
}

// Rozpoczyna rysowanie do sprita
void ExtendedTFT_eSPI::startDrawingToSprite()
{
    if (!sprite)
    {
        sprite = new TFT_eSprite(this);          // Powiąż sprite z bieżącym wyświetlaczem
        sprite->createSprite(width(), height()); // Utwórz bufor o wymiarach ekranu
    }
    sprite->fillSprite(TFT_BLACK); // Wyczyść bufor
}

// Wyświetla zawartość sprita na ekranie w punkcie (x, y)
void ExtendedTFT_eSPI::pushSpriteToScreen(int x, int y)
{
    if (sprite)
    {
        sprite->pushSprite(x, y); // Kopiuje bufor sprita na ekran
    }
}

// Usuwa sprita z pamięci
void ExtendedTFT_eSPI::endSprite()
{
    if (sprite)
    {
        sprite->deleteSprite();
        delete sprite;
        sprite = nullptr;
    }
}

// Przykład rysowania tekstu w spricie
void ExtendedTFT_eSPI::drawTextInSprite(const char *text, int x, int y, uint8_t fontSize)
{
    if (sprite)
    {
        sprite->setTextSize(fontSize);
        sprite->setTextColor(TFT_WHITE, TFT_BLACK); // Biały tekst na czarnym tle
        sprite->setCursor(x, y);
        sprite->print(text);
    }
}

void ExtendedTFT_eSPI::updateOTAProgress(int current, int total){
    int screenWidth = owidth();
    int screenHeight = oheight();
    
    // Wymiary paska postępu jako procent rozdzielczości
    int barWidth = screenWidth * 0.8;        // Szerokość paska to 80% szerokości ekranu
    int barHeight = screenHeight * 0.05;     // Wysokość paska to 5% wysokości ekranu
    int barX = (screenWidth - barWidth) / 2; // Centrowanie paska na ekranie (X)
    int barY = screenHeight / 2 - barHeight; // Umieszczanie paska mniej więcej w połowie ekranu (Y)

    // Obliczanie procentowego postępu
    float progress = (float)current / total;
    int filledWidth = (int)(barWidth * progress); // Szerokość wypełnionej części paska
    int percent = (int)(progress * 100);          // Obliczenie procentów
    if (percent == oldCurrentOTAProgress)
        return; //do not update when percentage did not change

    startDrawingToSprite();
    // Rysowanie tła paska postępu
    sprite->fillRect(barX, barY, barWidth, barHeight, TFT_DARKGREY);

    // Rysowanie wypełnionej części paska postępu
    sprite->fillRect(barX, barY, filledWidth, barHeight, TFT_GREEN);

    // Rysowanie ramki wokół paska
    sprite->drawRect(barX, barY, barWidth, barHeight, TFT_WHITE);

    // Wyświetlanie tekstu z procentami
    char buffer[15];
    snprintf(buffer, sizeof(buffer), "Updating %d%%", percent);

    // Pozycja tekstu poniżej paska postępu
    int textX = (screenWidth - textWidth("Updating xx%")) / 2; // Centrowanie tekstu
    int textY = barY + barHeight + 10;                     // 10 pikseli poniżej paska

    // Wyświetl tekst
    // sprite->setTextColor(TFT_WHITE, TFT_BLACK);
    drawTextInSprite(buffer, textX, textY, 1);
    // sprite->setCursor(textX, textY);
    // sprite->printf("Updating %d%%", percent);
    outputDebugf("Updating %d%%\n", percent);
    pushSpriteToScreen();
    endSprite();
    oldCurrentOTAProgress = percent;
}