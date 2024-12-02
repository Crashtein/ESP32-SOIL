#pragma once

#if DEBUG == 1
#define outputDebug(x) Serial.print(x)
#define outputDebugln(x) Serial.println(x)
#else
#define outputDebug(x)
#define outputDebugln(x)
#endif
