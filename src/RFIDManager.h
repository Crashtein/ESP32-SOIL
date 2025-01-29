#ifndef RFID_MANAGER_H
#define RFID_MANAGER_H

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include "Debug.h"

#ifndef RFID_SDA_PIN
#define RFID_SDA_PIN 21
#endif
#ifndef RFID_RST_PIN
#define RFID_RST_PIN 15
#endif
#ifndef RFID_MISO_PIN
#define RFID_MISO_PIN 13
#endif
#ifndef RFID_MOSI_PIN
#define RFID_MOSI_PIN 12
#endif
#ifndef RFID_SCK_PIN
#define RFID_SCK_PIN 22
#endif

#define PRETTY true
#define NO_PRETTY false
#define ALLOW_OLD_TAG false
#define NO_OLD_TAG true
#define INCLUDE_UID true
#define NOINCLUDE_UID false

class RFIDManager
{
private:
    static RFIDManager *instance;
    MFRC522 *rfid;
    WebServer *server;
    bool serverRunning = false;
    // Klucz domyślny dla kart Mifare
    MFRC522::MIFARE_Key key;

    // Konstruktor prywatny (singleton)
    RFIDManager();

    // Metody pomocnicze
    void setupWebServer();
    bool authenticateSector(byte sector);
    bool writeDataToBlock(byte blockAddr, byte *data);
    bool readDataFromBlock(byte blockAddr, byte *data);
    bool writeToTag(String data, bool validateJSON = false, bool newCardOnly = true, bool clearSectors = false);
    bool writeToTag(JsonDocument json, bool newCardOnly = true, bool clearSectors = false);
    bool wakeUpRFID();
    void stopRFIDCommunication();

public:
    static RFIDManager *
    getInstance();

    // Główne funkcje publiczne
    JsonDocument readJSONFromTag(bool newCardOnly = true);
    String readJSONStringFromTag(bool pretty = false, bool newCardOnly = true);
    String readStringFromTag(bool newCardOnly = true);
    bool copyDataSectors(bool includeUID = false, bool newCardOnly = false);
    bool copyUIDSector(bool newCardOnly = false);
    bool cloneTag(bool newCardOnly = true);
    void startConfigServer(void (*startConfigServerCallback)() = nullptr, void (*closeConfigServerCallback)() = nullptr);
    void stopWebServer();
    String getTagType(bool newCardOnly = true);
    void dumpInfoToSerial();
    void clearTag(bool newCardOnly);
    bool checkIfTagPresent(bool newCardOnly = true);
    bool checkIfJSONStringValid(String jsonString);
    String getTagUID(bool newCardOnly = false);

    // Destruktor
    ~RFIDManager();

    // Zabezpieczenie przed kopiowaniem
    RFIDManager(const RFIDManager &) = delete;
    RFIDManager &operator=(const RFIDManager &) = delete;
};

#endif // RFID_MANAGER_H