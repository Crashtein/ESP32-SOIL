#include "ExtendedTFT_eSPI.h"

#include <esp_heap_caps.h>

void printMemoryStatus()
{
    Serial.printf("Free Heap: %u bytes\n", esp_get_free_heap_size());
    Serial.printf("Free PSRAM: %u bytes\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

ExtendedTFT_eSPI &ExtendedTFT_eSPI::getInstance()
{
    static ExtendedTFT_eSPI instance;
    return instance;
}

ExtendedTFT_eSPI::ExtendedTFT_eSPI() : TFT_eSPI(), sprite(nullptr)
{
    // Dodaj debugowanie, aby upewnić się, że konstruktor jest wywoływany
    Serial.println("ExtendedTFT_eSPI constructor called");
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

void ExtendedTFT_eSPI::initDefault()
{
    TFT_eSPI::init();
    setRotation(1);
    fillScreen(TFT_BLACK);
    setTextColor(TFT_WHITE, TFT_BLACK);
    setTextSize(1);
    startDrawingToSprite();
}

void ExtendedTFT_eSPI::setRotation(int rotation)
{
    TFT_eSPI::setRotation(rotation);
}

// Rozpoczyna rysowanie do sprita
void ExtendedTFT_eSPI::startDrawingToSprite()
{
    if (!sprite)
    {
        sprite = new TFT_eSprite(this); // Powiąż sprite z bieżącym wyświetlaczem
        sprite->setColorDepth(8);
        if (sprite->createSprite(width(), height()) == nullptr)
            Serial.println("Error: sprite not created, not enough free RAM!");
        else
            Serial.println("Yahoo, success, sprite created!");
        // sprite->createSprite(width(), height()); // Utwórz bufor o wymiarach ekranu
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
    else
    {
        Serial.println("Error: sprite is not initialized!");
    }
}

void ExtendedTFT_eSPI::updateOTAProgressCallback(int current, int total)
{
    printMemoryStatus();
    int screenWidth = width();
    int screenHeight = height();

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
        return; // do not update when percentage did not change
    outputDebugln("DEBUG 1");
    startDrawingToSprite();
    outputDebugln("DEBUG 2");
    // Rysowanie tła paska postępu
    sprite->fillRect(barX, barY, barWidth, barHeight, TFT_DARKGREY);
    outputDebugln("DEBUG 3");
    // Rysowanie wypełnionej części paska postępu
    sprite->fillRect(barX, barY, filledWidth, barHeight, TFT_GREEN);
    outputDebugln("DEBUG 4");
    // Rysowanie ramki wokół paska
    sprite->drawRect(barX, barY, barWidth, barHeight, TFT_WHITE);
    outputDebugln("DEBUG 5");
    // Wyświetlanie tekstu z procentami
    char buffer[15];
    snprintf(buffer, sizeof(buffer), "Updating %d%%", percent);
    printMemoryStatus();
    // Pozycja tekstu poniżej paska postępu
    int textX = (screenWidth - textWidth(buffer)) / 2; // Centrowanie tekstu
    int textY = barY + barHeight + 10;                 // 10 pikseli poniżej paska
    outputDebugln("DEBUG 6");
    printMemoryStatus();
    // Wyświetl tekst - UWAGA - z jakiegoś powodu robi CORE PANIC
    // drawTextInSprite(buffer, textX, textY, 2);
    outputDebugf("Updating %d%%\n", percent);
    pushSpriteToScreen();
    oldCurrentOTAProgress = percent;
}

void ExtendedTFT_eSPI::printVersionInfo()
{
    startDrawingToSprite();
    sprite->setTextSize(2);
    sprite->setCursor(0, 8);
    sprite->printf("Version:\n%s\n", PROJECT_VERSION);
    sprite->setCursor(0, 40);
    sprite->printf("Compilation date:\n%s\n%s\n", __DATE__, __TIME__);
    pushSpriteToScreen();
}

void ExtendedTFT_eSPI::wifiAPcallback(WiFiManager *wm)
{
    fillScreen(TFT_BLACK);
    setTextSize(2);
    setCursor(0, 0);
    setTextColor(TFT_WHITE, TFT_BLACK);
    println("Configuration mode");
    setTextColor(TFT_WHITE, TFT_BLACK);
    print("Connect to WiFi:\n");
    setTextColor(TFT_YELLOW, TFT_BLACK);
    printf("%s\n", wm->getConfigPortalSSID());
    if (AP_PASSWORD)
    {
        printf("%s\n", AP_PASSWORD);
    }
    setTextColor(TFT_WHITE, TFT_BLACK);
    println("Pass in browser:");
    setTextColor(TFT_YELLOW, TFT_BLACK);
    printf("http://%s\n", WiFi.softAPIP().toString());
}