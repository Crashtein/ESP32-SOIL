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
    // Validate JSON if required
    if (validateJSON && !checkIfJSONStringValid(data))
    {
        outputDebugln("Invalid JSON data");
        return false;
    }

    // Check if tag is present and can be read
    if (!checkIfTagPresent(newCardOnly))
    {
        outputDebugln("Tag not present or cannot be read");
        return false;
    }

    // Variables for tracking position in data
    int currentSector = 1; // Start from sector 1 to preserve manufacturer block
    int currentPos = 0;
    int dataLength = data.length();

    // Process all sectors that need to be written or cleared
    while ((currentPos < dataLength || clearSectors) && currentSector < 16)
    {
        // Try to authenticate the current sector multiple times if needed
        int authAttempts = 0;
        bool authenticated = false;
        while (authAttempts < 3 && !authenticated)
        {
            if (authenticateSector(currentSector))
            {
                authenticated = true;
            }
            else
            {
                authAttempts++;
                delay(10); // Short delay between authentication attempts
            }
        }

        if (!authenticated)
        {
            outputDebugf("Failed to authenticate sector %d after multiple attempts\n", currentSector);
            stopRFIDCommunication();
            return false;
        }

        // Write to blocks 0-2 in the sector (block 3 is the sector trailer)
        for (int block = 0; block < 3; block++)
        {
            byte blockData[16] = {0}; // Initialize with zeros
            int blockAddr = currentSector * 4 + block;

            // Only copy data if we still have data to write
            if (currentPos < dataLength)
            {
                // Copy data to block buffer
                for (int i = 0; i < 16 && currentPos < dataLength; i++)
                {
                    blockData[i] = data[currentPos++];
                }
            }

            // Try to write block multiple times if needed
            int writeAttempts = 0;
            bool writeSuccess = false;
            while (writeAttempts < 3 && !writeSuccess)
            {
                if (writeDataToBlock(blockAddr, blockData))
                {
                    writeSuccess = true;
                }
                else
                {
                    writeAttempts++;
                    delay(10); // Short delay between write attempts
                }
            }

            if (!writeSuccess)
            {
                outputDebugf("Failed to write block %d in sector %d after multiple attempts\n", block, currentSector);
                stopRFIDCommunication();
                return false;
            }

            // Verify written data
            byte verifyData[18];
            if (readDataFromBlock(blockAddr, verifyData))
            {
                for (int i = 0; i < 16; i++)
                {
                    if (verifyData[i] != blockData[i])
                    {
                        outputDebugf("Data verification failed for block %d in sector %d\n", block, currentSector);
                        stopRFIDCommunication();
                        return false;
                    }
                }
            }
            else
            {
                outputDebugf("Failed to verify block %d in sector %d\n", block, currentSector);
                stopRFIDCommunication();
                return false;
            }
        }

        currentSector++;
    }

    // Properly end communication
    stopRFIDCommunication();
    outputDebugln("Write operation completed successfully");
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
    if(!wakeUpRFID())
    {
        outputDebugln("Could not wake up tag (may not be present)");
        return false;
    }
    if (!rfid->PICC_ReadCardSerial())
    {
        outputDebugln("Cannot read card serial");
        return false;
    }
    // if (rfid->PICC_Select(&(rfid->uid)) != MFRC522::STATUS_OK){
    //     outputDebugln("Cannot select PICC");
    //     return false;
    // }
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

bool RFIDManager::copyDataSectors(bool includeUID, bool newCardOnly)
{
    outputDebugln("Starting tag copying process...");

    // Check and authenticate source tag
    if (!checkIfTagPresent(newCardOnly))
    {
        outputDebugln("Source tag not present or cannot be read");
        return false;
    }

    // Dump debug info about source tag
    rfid->PICC_DumpDetailsToSerial(&(rfid->uid));

    MFRC522::PICC_Type sourceType = rfid->PICC_GetType(rfid->uid.sak);
    outputDebugf("Source tag type: %s\n", rfid->PICC_GetTypeName(sourceType));

    byte sourceData[64][16] = {0};

    outputDebugln("Reading source tag data...");

    // Read data from source tag
    for (int sector = (includeUID ? 0 : 1); sector < 16; sector++)
    {
        if (!authenticateSector(sector))
        {
            outputDebugf("Failed to authenticate source sector %d\n", sector);
            stopRFIDCommunication();
            return false;
        }

        for (int block = 0; block < 4; block++)
        {
            int blockAddr = sector * 4 + block;
            if (!readDataFromBlock(blockAddr, sourceData[blockAddr]))
            {
                outputDebugf("Failed to read block %d in sector %d\n", block, sector);
                stopRFIDCommunication();
                return false;
            }
            outputDebugf("Successfully read block %d in sector %d\n", block, sector);
        }
    }

    stopRFIDCommunication();
    outputDebugln("Successfully read all data from source tag");

    // Pełny reset RFID
    delete rfid;
    SPI.end();
    delay(500); // Zwiększone opóźnienie

    // Reinicjalizacja MFRC522
    SPI.begin(RFID_SCK_PIN, RFID_MISO_PIN, RFID_MOSI_PIN, RFID_SDA_PIN);
    rfid = new MFRC522(RFID_SDA_PIN, RFID_RST_PIN);
    rfid->PCD_Init();
    delay(500); // Zwiększone opóźnienie

    // Debug info o stanie czytnika
    byte v = rfid->PCD_ReadRegister(MFRC522::VersionReg);
    outputDebugf("MFRC522 Software Version: 0x%x\n", v);

    outputDebugln("Please remove the source tag and present destination tag...");
    delay(1000);

    // Wait for destination tag
    outputDebugln("Waiting for destination tag...");
    int waitAttempts = 0;
    const int maxWaitAttempts = 100;
    bool cardFound = false;

    while (waitAttempts < maxWaitAttempts && !cardFound)
    {
        delay(100);

        if (!rfid->PICC_IsNewCardPresent())
        {
            waitAttempts++;
            continue;
        }

        if (!rfid->PICC_ReadCardSerial())
        {
            waitAttempts++;
            continue;
        }

        cardFound = true;
        outputDebugln("New tag detected!");

        // Dump debug info about destination tag
        rfid->PICC_DumpDetailsToSerial(&(rfid->uid));

        MFRC522::PICC_Type destType = rfid->PICC_GetType(rfid->uid.sak);
        outputDebugf("Destination tag type: %s\n", rfid->PICC_GetTypeName(destType));
    }

    if (!cardFound)
    {
        outputDebugln("Timeout waiting for destination tag");
        return false;
    }

    // Reinicjalizacja klucza
    MFRC522::MIFARE_Key newKey;
    for (byte i = 0; i < 6; i++)
    {
        newKey.keyByte[i] = 0xFF;
    }

    outputDebugln("Attempting authentication with destination tag...");

    // Test authentication for sector 1 first
    byte testBlock = 4; // First block of sector 1
    outputDebugf("Testing authentication for block %d...\n", testBlock);

    // Debug info before authentication
    outputDebugf("Card UID: ");
    for (byte i = 0; i < rfid->uid.size; i++)
    {
        outputDebugf("%02X:", rfid->uid.uidByte[i]);
    }
    outputDebugln("");

    // Próba autoryzacji z dodatkowymi informacjami
    MFRC522::StatusCode status = rfid->PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A,
        testBlock,
        &newKey,
        &(rfid->uid));

    if (status != MFRC522::STATUS_OK)
    {
        outputDebugf("Initial authentication failed. Status = %d\n", status);
        outputDebugln("Attempting recovery process...");

        // Próba odzyskania komunikacji
        rfid->PCD_StopCrypto1();
        delay(100);
        rfid->PCD_Reset();
        delay(100);
        rfid->PCD_Init();
        delay(100);

        if (!rfid->PICC_IsNewCardPresent() || !rfid->PICC_ReadCardSerial())
        {
            outputDebugln("Recovery failed - cannot redetect card");
            return false;
        }

        status = rfid->PCD_Authenticate(
            MFRC522::PICC_CMD_MF_AUTH_KEY_A,
            testBlock,
            &newKey,
            &(rfid->uid));

        if (status != MFRC522::STATUS_OK)
        {
            outputDebugf("Recovery authentication failed. Status = %d\n", status);
            return false;
        }
    }

    outputDebugln("Initial authentication successful, proceeding with copy...");

    // Write data to destination tag
    for (int sector = (includeUID ? 0 : 1); sector < 16; sector++)
    {
        rfid->PCD_StopCrypto1();
        delay(50);

        status = rfid->PCD_Authenticate(
            MFRC522::PICC_CMD_MF_AUTH_KEY_A,
            sector * 4,
            &newKey,
            &(rfid->uid));

        if (status != MFRC522::STATUS_OK)
        {
            outputDebugf("Failed to authenticate sector %d. Status = %d\n", sector, status);
            stopRFIDCommunication();
            return false;
        }

        for (int block = 0; block < 3; block++)
        {
            int blockAddr = sector * 4 + block;

            if (!writeDataToBlock(blockAddr, sourceData[blockAddr]))
            {
                outputDebugf("Failed to write block %d in sector %d\n", block, sector);
                stopRFIDCommunication();
                return false;
            }

            byte verifyData[18];
            if (!readDataFromBlock(blockAddr, verifyData))
            {
                outputDebugf("Failed to verify block %d in sector %d\n", block, sector);
                stopRFIDCommunication();
                return false;
            }

            bool verifyOk = true;
            for (int i = 0; i < 16; i++)
            {
                if (verifyData[i] != sourceData[blockAddr][i])
                {
                    verifyOk = false;
                    break;
                }
            }

            if (!verifyOk)
            {
                outputDebugf("Data verification failed for block %d in sector %d\n", block, sector);
                stopRFIDCommunication();
                return false;
            }

            outputDebugf("Successfully wrote and verified block %d in sector %d\n", block, sector);
        }
    }

    stopRFIDCommunication();
    outputDebugln("Copy operation completed successfully");
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