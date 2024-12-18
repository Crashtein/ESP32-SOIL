#include "PinManager.h"

PinManager::PinManager()
{
    // Konstruktor domyślny (pusty, brak pinów na starcie)
}

PinManager &PinManager::getInstance()
{
    static PinManager instance; // Singleton
    return instance;
}

void PinManager::begin(const std::vector<uint8_t> &digitalPins, const std::vector<uint8_t> &analogPins)
{
    // Przypisz do listy pinów
    for (uint8_t pin : digitalPins)
    {
        PinState state;
        state.pin = pin;
        state.currentState = LOW;
        state.lastState = LOW;
        state.stateChanged = false;

        // Dodaj pin do listy monitorowanych pinów
        digitalPinStates.push_back(state);

        // Ustaw tryb pinu jako wejście
        pinMode(pin, INPUT);
    }
    for (uint8_t pin : analogPins)
    {
        // Ustaw tryb pinu jako wejście
        pinMode(pin, INPUT);
    }
}

void PinManager::update()
{
    // Sprawdzamy zmiany na pinach
    for (auto &pinState : digitalPinStates)
    {
        bool currentState = digitalRead(pinState.pin);
        if (pinState.currentState != currentState)
        {
            // Stan pinu się zmienił
            pinState.lastState = pinState.currentState;
            pinState.currentState = currentState;
            pinState.stateChanged = true;
        }
        else
        {
            pinState.stateChanged = false;
        }
    }
}

bool PinManager::readDigitalPin(uint8_t pin)
{
    for (const auto &pinState : digitalPinStates)
    {
        if (pinState.pin == pin)
        {
            return pinState.currentState == HIGH;
        }
    }
    return false; // Jeśli pin nie został znaleziony
}

bool PinManager::digitalStateChanged(uint8_t pin)
{
    for (const auto &pinState : digitalPinStates)
    {
        if (pinState.pin == pin)
        {
            return pinState.stateChanged;
        }
    }
    return false; // Jeśli pin nie został znaleziony
}

bool PinManager::anyDigitalStateChanged(){
    for (const auto &pinState : digitalPinStates)
    {
        if (pinState.stateChanged)
        {
            return true;
        }
    }
    return false;
}

int PinManager::readAnalogPin(uint8_t pin)
{
    return analogRead(pin); // Zwraca odczyt analogowy
}

void PinManager::debug()
{
    // Wypisujemy informacje o wszystkich pinach w Serial Monitorze
    Serial.println("Pin Manager Debug:");

    for (const auto &pinState : digitalPinStates)
    {
        Serial.print("Pin: ");
        Serial.print(pinState.pin);
        Serial.print(" | Current state: ");
        Serial.print(pinState.currentState ? "HIGH" : "LOW");
        Serial.print(" | Last state: ");
        Serial.print(pinState.lastState ? "HIGH" : "LOW");
        Serial.print(" | State Changed: ");
        Serial.println(pinState.stateChanged ? "YES" : "NO");
    }
}