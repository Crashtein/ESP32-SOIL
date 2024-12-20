#ifndef PIN_MANAGER_H
#define PIN_MANAGER_H

#include "Arduino.h"
#include <vector>

// Struktura przechowująca stan pinu wraz z numerem pinu
struct PinState
{
    uint8_t pin;       // Numer pinu
    bool currentState; // Bieżący stan pinu (wysoki/niski)
    bool lastState;    // Poprzedni stan pinu
    bool stateChanged; // Flaga informująca, czy stan pinu się zmienił
};

class PinManager
{
public:
    // Publiczna metoda do uzyskania instancji singletonu
    static PinManager &getInstance();

    // Funkcja do inicjalizacji pinów. Przyjmuje tablicę numerów pinów, które mają być monitorowane.
    void begin(const std::vector<uint8_t> &digitalPins = {}, const std::vector<uint8_t> &analogPins = {});

    // Funkcja do aktualizacji stanów pinów i wykrywania zmian
    void update();

    // Funkcja do odczytu stanu pinu cyfrowego
    bool readDigitalPin(uint8_t pin);

    // Funkcja do odczytu stanu zmiany pinu cyfrowego
    bool digitalStateChanged(uint8_t pin);

    // Funkcja zwracająca true jeśli jakikolwiek pin cyfrowy zmienił stan
    bool anyDigitalStateChanged();

    // Funkcja do odczytu z pinów analogowych
    int readAnalogPin(uint8_t pin);

    // Funkcja do wypisania debugowych informacji o pinach
    void debug();

    // Funkcja zwracająca czas ostatniej zmiany na jakimkolwiek przycisku
    unsigned long getLastActivityTime() { return lastActivityTime; }

private:
    // Konstruktor prywatny
    PinManager();

    // Dynamiczna lista przechowująca stany monitorowanych pinów
    std::vector<PinState> digitalPinStates;

    // Zapewnia, że nie będzie duplikatów instancji (wyłączamy kopię konstruktora i operator przypisania)
    PinManager(const PinManager &) = delete;
    PinManager &operator=(const PinManager &) = delete;

    //last activity time (changes in any pin)
    unsigned long lastActivityTime = 0;
};

#endif // PIN_MANAGER_H
