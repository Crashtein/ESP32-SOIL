#include "debug.h"

void printMemoryStatus()
{
    Serial.printf("Free Heap: %u bytes\n", esp_get_free_heap_size());
    Serial.printf("Free PSRAM: %u bytes\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}
