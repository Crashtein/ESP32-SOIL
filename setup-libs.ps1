#choco install -y sed
# dev
sed -i '/^#include <User_Setup.h>/a#include <User_Setups/Setup25_TTGO_T_Display.h>' .pio/libdeps/dev/TFT_eSPI/User_Setup_Select.h
sed -i 's|^#include <User_Setup.h>|//#include <User_Setup.h>|' .pio/libdeps/dev/TFT_eSPI/User_Setup_Select.h

sed -i 's|backData && (backLen > 0)|backData != nullptr \&\& backLen != nullptr \&\& *backLen > 0|g' .pio/libdeps/dev/MFRC522/src/MFRC522Extended.cpp #fix error on MFRC522 lib to allow compilation
# release
sed -i '/^#include <User_Setup.h>/a#include <User_Setups/Setup25_TTGO_T_Display.h>' .pio/libdeps/release/TFT_eSPI/User_Setup_Select.h
sed -i 's|^#include <User_Setup.h>|//#include <User_Setup.h>|' .pio/libdeps/release/TFT_eSPI/User_Setup_Select.h

sed -i 's|backData && (backLen > 0)|backData != nullptr \&\& backLen != nullptr \&\& *backLen > 0|g' .pio/libdeps/release/MFRC522/src/MFRC522Extended.cpp #fix error on MFRC522 lib to allow compilation
