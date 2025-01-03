#pragma once // Używanie pragma once zamiast #ifndef
#ifndef DEBUG_H
#define DEBUG_H

#if DEBUG == 1
#define outputDebug(...) Serial.print(__VA_ARGS__)
#define outputDebugln(...) Serial.println(__VA_ARGS__)
#define outputDebugf(...) Serial.printf(__VA_ARGS__)
#else
#define outputDebug(...)
#define outputDebugln(...)
#define outputDebugf(...)
#endif

#include <esp_heap_caps.h>
#include "Arduino.h"

void printMemoryStatus();

#endif