#include "ExtendedTFT_eSPI.h"
#ifndef BATTERY_VOLTAGE_CORRECTION
#define BATTERY_VOLTAGE_CORRECTION 1.0f
#endif

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
    if (oldCurrentOTAProgress == -1)
    {
        fillScreen(TFT_BLACK);
        // Rysowanie tła paska postępu
        fillRect(barX, barY, barWidth, barHeight, TFT_DARKGREY);
        // Rysowanie ramki wokół paska
        drawRect(barX - 1, barY - 1, barWidth + 2, barHeight + 2, TFT_WHITE);
    }

    // Rysowanie wypełnionej części paska postępu
    fillRect(barX, barY, filledWidth, barHeight, TFT_GREEN);
    // Wyświetlanie tekstu z procentami
    char buffer[15];
    snprintf(buffer, sizeof(buffer), "Updating %d%%", percent);
    // Pozycja tekstu poniżej paska postępu
    setTextSize(2);
    int textX = (screenWidth - textWidth(buffer)) / 2; // Centrowanie tekstu
    int textY = barY + barHeight + 10;                 // 10 pikseli poniżej paska
    // Wyświetl tekst - UWAGA - z jakiegoś powodu robi CORE PANIC
    drawString(buffer, textX, textY, 2);
    outputDebugf("Updating %d%%\n", percent);
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

void ExtendedTFT_eSPI::drawStatusBar()
{
    setTextSize(1);
    setTextColor(TFT_YELLOW, TFT_BLACK);
    setCursor(0, 0);
    printf("H:%3.1f%% T:%3.1f÷C", DHT22Reader::getInstance()->getHumidity(), DHT22Reader::getInstance()->getTemperature());
    setCursor(145, 0);

    if (WifiConfig::getInstance().getStatus())
    {
        setTextColor(TFT_GREEN, TFT_BLACK);
    }
    else
    {
        setTextColor(TFT_RED, TFT_BLACK);
    }
    print("WiFi");
    setTextColor(TFT_WHITE, TFT_BLACK);
    print("|");
    if (MQTTManager::getInstance().getStatus())
    {
        setTextColor(TFT_GREEN, TFT_BLACK);
    }
    else
    {
        setTextColor(TFT_RED, TFT_BLACK);
    }
    print("MQTT");
    setTextColor(TFT_WHITE, TFT_BLACK);
    print("|");
    setCursor(206, 0);
    float batteryVoltage = BATTERY_VOLTAGE_CORRECTION * 2.0f * (analogRead(BATTERY_VOLTAGE_PIN) * 3.3f) / 4095.0f;
    // smooth battery voltage readings
    batteryVoltage = (4 * oldBatteryVoltage + batteryVoltage) / 5;
    oldBatteryVoltage = batteryVoltage;
    if (batteryVoltage > 4.2)
    {
        setTextColor(TFT_BLUE, TFT_BLACK);
    }
    else if (batteryVoltage > 3.7)
    {
        setTextColor(TFT_GREEN, TFT_BLACK);
    }
    else if (batteryVoltage > 3.4)
    {
        setTextColor(TFT_YELLOW, TFT_BLACK);
    }
    else
    {
        setTextColor(TFT_RED, TFT_BLACK);
    }
    printf("%3.2fV", batteryVoltage);
}

void ExtendedTFT_eSPI::printProgramInfo()
{
    setTextSize(2);
    setCursor(0, 8);
    printf("Version:\n%s\n", PROJECT_VERSION);
    setCursor(0, 40);
    printf("Compilation date:\n%s\n%s\n", __DATE__, __TIME__);
}

void ExtendedTFT_eSPI::printSleepInfo()
{
    clear();
    setTextSize(4);
    setCursor(0, 50);
    println("SLEEP MODE");
}

void ExtendedTFT_eSPI::showMenuOption(const int currentIndex, const std::vector<String> currentMenuNames, const std::vector<bool> checkedVector)
{
    clear(); // Clear the screen
    drawStatusBar(); // Draw the status bar

    const int skippedPixelsOnTop = 8;
    const int textSize = 2;

    setTextSize(textSize); // Set text size

    int lineHeight = 8*textSize+2; // Height of each line (adjust as needed)
    int screenHeight = height()-skippedPixelsOnTop; // Get the height of the screen
    int maxLines = screenHeight / lineHeight; // Calculate the maximum number of lines that can fit on the screen

    int startLine = 0; // Start line for displaying menu items
    if (currentIndex >= maxLines-1)
    { // Adjust start line if the selected item is beyond the screen height
        startLine = currentIndex - maxLines + 1;
        if(currentIndex < (currentMenuNames.size()-1)){
            startLine++;
        }
    }

    for (int i = startLine; i < currentMenuNames.size() && i < startLine + maxLines; ++i) {
        int y = (i - startLine) * lineHeight + skippedPixelsOnTop; // Calculate the y position for the current line

        if (i == currentIndex) {
            if(checkedVector[i] == true){
                setTextColor(TFT_GREEN, TFT_BLUE);
            }else{
                setTextColor(TFT_BLACK, TFT_WHITE);
            } 
        } else {
            if (checkedVector[i] == true)
            {
                setTextColor(TFT_GREEN, TFT_BLACK);
            }
            else
            {
                setTextColor(TFT_WHITE, TFT_BLACK);
            }
        }

        setCursor((width() - textWidth(currentMenuNames[i])) / 2, y); // Center the text horizontally
        print(currentMenuNames[i]); // Print the menu item
    }
}

