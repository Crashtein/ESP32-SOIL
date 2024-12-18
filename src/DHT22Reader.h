#ifndef DHT22READER_H
#define DHT22READER_H

#include <Arduino.h>
#include "DHT.h" // Biblioteka do obsługi DHT22

#define DHT_PIN 33
#define DHT_TYPE DHT22
#define DHT_MIN_INTERVAL 2000

class DHT22Reader
{
private:
    static DHT22Reader *instance;               // Wskaźnik na jedyny obiekt klasy
    DHT dhtSensor;                              // Obiekt czujnika DHT
    float temperature;                          // Przechowywana wartość temperatury
    float humidity;                             // Przechowywana wartość wilgotności
    unsigned long lastReadTime;                 // Czas ostatniego odczytu
    const unsigned long refreshInterval = DHT_MIN_INTERVAL; // Minimalny interwał 2s

    // Prywatny konstruktor - tylko dla Singletona
    DHT22Reader(uint8_t pin, uint8_t type);

    // Prywatna funkcja odczytująca dane z czujnika
    void readFromSensor();

public:
    // Funkcja dostępu do instancji Singletona
    static DHT22Reader *getInstance(uint8_t pin = DHT_PIN, uint8_t type = DHT_TYPE);

    // Funkcja zwracająca temperaturę
    float getTemperature();

    // Funkcja zwracająca wilgotność
    float getHumidity();
};

#endif // DHT22READER_H
