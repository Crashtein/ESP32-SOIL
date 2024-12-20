#pragma once // Używanie pragma once zamiast #ifndef
#ifndef DEBUG_H
#define DEBUG_H

#if DEBUG == 1
#define outputDebug(x) Serial.print(x)
#define outputDebugln(x) Serial.println(x)
#define outputDebugf(...) Serial.printf(__VA_ARGS__)
#else
#define outputDebug(x)
#define outputDebugln(x)
#define outputDebugf(...)
#endif

#include <esp_heap_caps.h>
#include "Arduino.h"

void printMemoryStatus();

#endif