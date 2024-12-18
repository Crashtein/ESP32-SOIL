#include "DHT22Reader.h"

DHT22Reader *DHT22Reader::instance = nullptr; // Inicjalizacja statycznego wskaźnika Singletona

// Prywatny konstruktor
DHT22Reader::DHT22Reader(uint8_t pin, uint8_t type)
    : dhtSensor(pin, type), temperature(0.0), humidity(0.0), lastReadTime(0)
{
    dhtSensor.begin();
}

// Funkcja dostępu do instancji Singletona
DHT22Reader *DHT22Reader::getInstance(uint8_t pin, uint8_t type)
{
    if (instance == nullptr)
    {
        instance = new DHT22Reader(pin, type);
    }
    return instance;
}

// Prywatna funkcja odczytująca dane z czujnika
void DHT22Reader::readFromSensor()
{
    float newHumidity = dhtSensor.readHumidity();
    float newTemperature = dhtSensor.readTemperature();

    if (!isnan(newHumidity) && !isnan(newTemperature))
    { // Sprawdź poprawność odczytów
        humidity = newHumidity;
        temperature = newTemperature;
        lastReadTime = millis();
    }
}

// Funkcja zwracająca temperaturę
float DHT22Reader::getTemperature()
{
    if (millis() - lastReadTime >= refreshInterval)
    {
        readFromSensor();
    }
    return temperature;
}

// Funkcja zwracająca wilgotność
float DHT22Reader::getHumidity()
{
    if (millis() - lastReadTime >= refreshInterval)
    {
        readFromSensor();
    }
    return humidity;
}
