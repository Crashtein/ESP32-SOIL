#include "OTAUpdater.h"

OTAUpdater::OTAUpdater(){
    // set deafault CA certificate
    setServerCertificate(Certificates::ROOT_CA);
    // set default onProgress callback
    onProgress(defaultCallbackOnProgressToSerial);
}

void OTAUpdater::setupSecureClient()
{
    if (serverCertificate)
    {
        outputDebugln("Using provided certificate for HTTPS");
        client.setCACert(serverCertificate);
    }
    else
    {
        outputDebugln("Warning: Using insecure HTTPS");
        client.setInsecure();
    }
}

bool OTAUpdater::beginUpdate(const char *url, const char *currentVersion)
{
    outputDebugln("Starting OTA update...");

    if (!WiFi.isConnected())
    {
        outputDebugln("Error: WiFi not connected");
        return false;
    }

    setupSecureClient();

    if (currentVersion)
    {
        // Tutaj można dodać logikę sprawdzania wersji
        outputDebug("Current firmware version: ");
        outputDebugln(currentVersion);
    }

    return performUpdate(url);
}

bool OTAUpdater::performUpdate(const char *url)
{
    outputDebug("Updating from URL: ");
    outputDebugln(url);

    // Konfiguracja parametrów aktualizacji
    httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    httpUpdate.rebootOnUpdate(false); // Ręczny restart po aktualizacji

    // Rozpoczęcie aktualizacji
    t_httpUpdate_return ret = httpUpdate.update(client, url);

    switch (ret)
    {
    case HTTP_UPDATE_FAILED:
        outputDebug("HTTP_UPDATE_FAILED Error (");
        outputDebug(httpUpdate.getLastError());
        outputDebug("): ");
        outputDebugln(httpUpdate.getLastErrorString().c_str());
        return false;

    case HTTP_UPDATE_NO_UPDATES:
        outputDebugln("HTTP_UPDATE_NO_UPDATES");
        return false;

    case HTTP_UPDATE_OK:
        outputDebugln("HTTP_UPDATE_OK");
        outputDebugln("Update successful! Rebooting...");
        delay(1000);
        ESP.restart();
        return true;

    default:
        outputDebugln("Unexpected update result");
        return false;
    }
}

void OTAUpdater::defaultCallbackOnProgressToSerial(int progress, int total){
    Serial.printf("Progress: %d%%\n", (progress / (total / 100)));
}

void OTAUpdater::printInfo()
{
    Serial.printf("Software version: %s\n", PROJECT_VERSION);
    Serial.printf("Compilation date: %s %s\n", __DATE__, __TIME__);
}