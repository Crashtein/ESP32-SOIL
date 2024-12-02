#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include "Debug.h"
#include "Certificates.h"

class OTAUpdater
{
public:
    static OTAUpdater &getInstance()
    {
        static OTAUpdater instance;
        return instance;
    }

    bool beginUpdate(const char *url = OTA_URL, const char *currentVersion = PROJECT_VERSION);
    void setServerCertificate(const char *cert) { serverCertificate = cert; }
    void onProgress(void (*cb)(int, int)) { Update.onProgress(cb); }
    static void defaultCallbackOnProgressToSerial(int progress, int total);
    void printInfo();

private:
    OTAUpdater();
    const char *serverCertificate = nullptr;
    WiFiClientSecure client;

    bool performUpdate(const char *url);
    void setupSecureClient();
};
