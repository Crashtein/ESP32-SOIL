#include "RFIDManager.h"

// Inicjalizacja statycznego wskaźnika
RFIDManager *RFIDManager::instance = nullptr;

RFIDManager::RFIDManager()
{
    // Inicjalizacja klucza
    for (byte i = 0; i < 6; i++)
    {
        key.keyByte[i] = 0xFF;
    }

    // Inicjalizacja dedykowanego SPI dla MFRC522 (HSPI)
    SPI.begin(RFID_SCK_PIN, RFID_MISO_PIN, RFID_MOSI_PIN, RFID_SDA_PIN);
    // Inicjalizacja MFRC522 z wykorzystaniem SPI i zdefiniowanych pinów
    rfid = new MFRC522(RFID_SDA_PIN, RFID_RST_PIN);
    // Inicjalizacja urządzenia RFID
    rfid->PCD_Init(); // Init MFRC522
    delay(4);         // Optional delay. Some board do need more time after init to be ready, see Readme
#if DEBUG != 0
    rfid->PCD_DumpVersionToSerial();
#endif

    // Inicjalizacja serwera webowego
    server = new WebServer(80);
    setupWebServer();
}

RFIDManager *RFIDManager::getInstance()
{
    if (instance == nullptr)
    {
        instance = new RFIDManager();
    }
    return instance;
}

void RFIDManager::setupWebServer()
{
    server->on("/", HTTP_GET, [this]()
               {
        String html = "<html><head>"
                     "<style>"
                     "body { font-family: Arial, sans-serif; margin: 20px; }"
                     "textarea { width: 100%; padding: 10px; margin: 10px 0; }"
                     ".button { padding: 10px 20px; margin: 5px; cursor: pointer; }"
                     ".save { background-color: #4CAF50; color: white; border: none; }"
                     ".close { background-color: #f44336; color: white; border: none; }"
                     "</style>"
                     "</head><body>"
                     "<h2>RFID Tag Configuration</h2>"
                     "<form id='configForm' method='post' action='/save'>"
                     "<textarea name='json' rows='10' placeholder='Enter JSON data here...'></textarea><br>"
                     "<input type='submit' value='Save to Tag' class='button save'>"
                     "</form>"
                     "<form method='post' action='/read'>"
                     "<input type='submit' value='Read data from tag' class='button save'>"
                     "</form>"
                     "<form method='post' action='/close'>"
                     "<input type='submit' value='Close Server' class='button close'>"
                     "</form>"
                     "</body></html>";
        server->send(200, "text/html", html); });

    server->on("/save", HTTP_POST, [this]()
               {
        String jsonData = server->arg("json");
        if (writeToTag(jsonData,true,false)) {
            auto readJSON = readJSONStringFromTag(true,false);
            outputDebugf("Write JSON control: %s\n", readJSON);
            rfid->PICC_DumpToSerial(&(rfid->uid));
            server->send(200, "text/plain", "Data saved successfully, control read:\n" + readJSON);
        } else {
            server->send(500, "text/plain", "Error saving data");
        } });

    server->on("/close", HTTP_POST, [this]()
               {
        server->send(200, "text/plain", "Server is shutting down...");
        delay(500); // Daj czas na wysłanie odpowiedzi
        stopWebServer(); });

    server->on("/read", HTTP_POST, [this]()
               {
        auto readJSON = readJSONStringFromTag(true,false);
        if (readJSON!="null") {
            outputDebugf("Read JSON:\n %s\n",readJSON.c_str());
            server->send(200, "text/plain", "Data read successfully: \n"+readJSON);
        } else {
            server->send(500, "text/plain", "Error reading data");
        } });
}

void RFIDManager::stopWebServer()
{
    server->close();
    server->stop();
    serverRunning = false;
}

bool RFIDManager::authenticateSector(byte sector)
{
    byte blockAddr = sector * 4;
    return rfid->PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(rfid->uid)) == MFRC522::STATUS_OK;
}

bool RFIDManager::writeDataToBlock(byte blockAddr, byte *data)
{
    return rfid->MIFARE_Write(blockAddr, data, 16) == MFRC522::STATUS_OK;
}

bool RFIDManager::readDataFromBlock(byte blockAddr, byte *data)
{
    byte size = 18;
    return rfid->MIFARE_Read(blockAddr, data, &size) == MFRC522::STATUS_OK;
}

bool RFIDManager::checkIfJSONStringValid(String jsonString)
{
    JsonDocument json;
    if (deserializeJson(json, jsonString))
    {
        outputDebugln("Could not build JSON from data");
        return false;
    }
    return true;
}

bool RFIDManager::writeToTag(JsonDocument json, bool newCardOnly, bool clearSectors)
{
    String jsonString;
    if(deserializeJson(json,jsonString))
        return false;
    return writeToTag(jsonString, true, newCardOnly, clearSectors);
}

bool RFIDManager::writeToTag(String data, bool validateJSON, bool newCardOnly, bool clearSectors)
{
    if(validateJSON && !checkIfJSONStringValid(data))
        return false;
    if (!checkIfTagPresent(newCardOnly))
        return false;

    // Zapisz dane w sektorach od 1 do 15 (pomijając sektor 0 z ID)
    int currentSector = 1;
    int currentPos = 0;

    while ((currentPos < data.length() || clearSectors) && currentSector < 16)
    {
        if (!authenticateSector(currentSector))
        {
            return false;
        }

        for (int block = 0; block < 3; block++)
        {
            byte blockData[16] = {0};
            int blockAddr = currentSector * 4 + block;

            // Kopiuj dane do bloku
            for (int i = 0; i < 16 && currentPos < data.length(); i++)
            {
                blockData[i] = data[currentPos++];
            }

            if (!writeDataToBlock(blockAddr, blockData))
            {
                return false;
            }
        }
        currentSector++;
    }
    stopRFIDCommunication();
    return true;
}

bool RFIDManager::wakeUpRFID()
{
    byte bufferATQA[2];
    byte bufferSize = sizeof(bufferATQA);

    // Wybudzenie tagu
    MFRC522::StatusCode status = rfid->PICC_WakeupA(bufferATQA, &bufferSize);
    if (status == MFRC522::STATUS_OK)
    {
        #if DEBUG!=0
        outputDebug("Tag wzbudzony. ATQA: ");
        for (byte i = 0; i < bufferSize; i++)
        {
            outputDebug(bufferATQA[i], HEX);
            if (i < bufferSize - 1)
            {
                outputDebug(":");
            }
        }
        outputDebug('\n');
        #endif
        return true;
    }
    return false;
}

void RFIDManager::stopRFIDCommunication()
{
    rfid->PICC_HaltA();
    rfid->PCD_StopCrypto1();
}

String RFIDManager::readStringFromTag(bool newCardOnly){
    String data;
    if(!checkIfTagPresent(newCardOnly))
        return data;
    // Czytaj dane z sektorów 1-15
    for (int sector = 1; sector < 16; sector++)
    {
        if (!authenticateSector(sector))
        {
            continue;
        }

        for (int block = 0; block < 3; block++)
        {
            byte blockData[18];
            int blockAddr = sector * 4 + block;

            if (readDataFromBlock(blockAddr, blockData))
            {
                for (int i = 0; i < 16; i++)
                {
                    if (blockData[i] != 0)
                    {
                        data += (char)blockData[i];
                    }
                }
            }
        }
    }
    stopRFIDCommunication();
    return data;
}

JsonDocument RFIDManager::readJSONFromTag(bool newCardOnly)
{
    JsonDocument json;
    String data = readJSONStringFromTag(false,newCardOnly);
    if (deserializeJson(json, data))
        json.clear();
    stopRFIDCommunication();
    return json;
}

bool RFIDManager::checkIfTagPresent(bool newCardOnly)
{
    if (newCardOnly && !rfid->PICC_IsNewCardPresent())
    {
        outputDebugln("No new tag found");
        return false;
    }
    while (!wakeUpRFID())
    {
        if (rfid->PICC_Select(&(rfid->uid))!=MFRC522::STATUS_OK)
            break;
        outputDebugln("Could not wake up tag (may not be present)");
        return false;
    }
    if (!rfid->PICC_ReadCardSerial())
    {
        outputDebugln("Cannot read card serial");
        return false;
    }
    return true;
}

String RFIDManager::readJSONStringFromTag(bool pretty, bool newCardOnly)
{
    auto jsonString = readStringFromTag(newCardOnly);
    JsonDocument json;
    if (!checkIfJSONStringValid(jsonString))
        return "null";
    if (pretty)
    {
        deserializeJson(json, jsonString);
        serializeJsonPretty(json, jsonString);
    }
    return jsonString;
}

bool RFIDManager::copyDataSectors(bool includeUID,bool newCardOnly)
{
    if(!checkIfTagPresent(newCardOnly))
        return false;
    MFRC522::PICC_Type sourceType = rfid->PICC_GetType(rfid->uid.sak);
    byte sourceData[64][16];
    rfid->PICC_Select(&(rfid->uid));
    outputDebugln("Reading tag data");
    // Czytaj dane z źródłowego tagu
    for (int sector = (includeUID ? 0 : 1); sector < 16; sector++)
    {
        if (!authenticateSector(sector))
        {
            return false;
        }

        for (int block = 0; block < 4; block++)
        {
            if (!readDataFromBlock(sector * 4 + block, sourceData[sector * 4 + block]))
            {
                return false;
            }
        }
    }
    stopRFIDCommunication();
    outputDebugln("Waiting to new tag");
    // Czekaj na nowy tag
    delay(2000);
    while (!rfid->PICC_IsNewCardPresent() || !rfid->PICC_ReadCardSerial())
    {
        delay(100);
    }
    if (rfid->PICC_Select(&(rfid->uid))){
        outputDebugln("Could not select PICC");
        return false;
    }
    // Sprawdź czy nowy tag jest tego samego typu
    if (!checkIfTagPresent(false) && rfid->PICC_GetType(rfid->uid.sak) != sourceType)
    {
        return false;
    }
    outputDebugln("Writing data to new tag");
    // Zapisz dane na nowym tagu
    for (int sector = (includeUID ? 0 : 1); sector < 16; sector++)
    {
        if (!authenticateSector(sector))
        {
            outputDebugln("Write data error when authenicating to sector");
            return false;
        }

        for (int block = 0; block < 4; block++)
        {
            if (!writeDataToBlock(sector * 4 + block, sourceData[sector * 4 + block]))
            {
                outputDebugln("Write data error when writing to block");
                return false;
            }
        }
    }
    stopRFIDCommunication();
    return true;
}

bool RFIDManager::copyUIDSector(bool newCardOnly)
{
    if (!checkIfTagPresent(newCardOnly))
    {
        return false;
    }

    byte uidData[16];
    if (!readDataFromBlock(0, uidData))
    {
        return false;
    }

    while (!rfid->PICC_IsNewCardPresent() || !rfid->PICC_ReadCardSerial())
    {
        delay(100);
    }

    return writeDataToBlock(0, uidData);
}

bool RFIDManager::cloneTag(bool newCardOnly)
{
    return copyDataSectors(true,newCardOnly);
}

void RFIDManager::startConfigServer(void (*startConfigServerCallback)(), void (*closeConfigServerCallback)())
{
    outputDebugf("Starting webserver on: http://%s/\n", WiFi.localIP().toString());
    server->begin();
    serverRunning = true;
    startConfigServerCallback();
    while (serverRunning)
    {
        server->handleClient();
        delay(10);
    }
    closeConfigServerCallback();
}

String RFIDManager::getTagType(bool newCardOnly)
{
    if (!checkIfTagPresent(newCardOnly))
    {
        return "No card present";
    }

    MFRC522::PICC_Type piccType = rfid->PICC_GetType(rfid->uid.sak);
    stopRFIDCommunication();
    return String(rfid->PICC_GetTypeName(piccType));
}

RFIDManager::~RFIDManager()
{
    delete rfid;
    delete server;
}

void RFIDManager::dumpInfoToSerial()
{
    // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
    if (!checkIfTagPresent(false))
    {
        return;
    }

    // Dump debug info about the card; PICC_HaltA() is automatically called
    rfid->PICC_DumpToSerial(&(rfid->uid));
    stopRFIDCommunication();
}

void RFIDManager::clearTag(bool newCardOnly)
{
    if(writeToTag("", false, newCardOnly,true)){
        outputDebugln("Tag data cleared");
    }else{
        outputDebugln("Could not cleared tag data");
    }
    return;
}

String RFIDManager::getTagUID(bool newCardOnly){
    
    
    // Bufor na ID tagu RFID
    String tagID = "";
    if(!checkIfTagPresent(newCardOnly))
        return tagID;
    // Skonwertowanie numeru seryjnego na ciąg znaków (hex)
    for (byte i = 0; i < rfid->uid.size; i++)
    {
        if (rfid->uid.uidByte[i] < 0x10)
        {
            tagID += "0"; // Dodanie zera wiodącego dla wartości < 16
        }
        tagID += String(rfid->uid.uidByte[i], HEX);
    }

    tagID.toUpperCase(); // Zamień na wielkie litery
    stopRFIDCommunication();
    return tagID;
}