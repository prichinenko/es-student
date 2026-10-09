#pragma once

#include <stdint.h>

#define DEVICE_NAME "es-cmd-usb"
#define FIRMWARE_VERSION "1.1.0"

#define DEVICE_PROJECT "231-firmware-template"
#define DEVICE_REPO "https://github.com/prichinenko/es-student"

#ifndef DEVICE_BOARD
#define DEVICE_BOARD "unknown"
#endif

#ifndef PICO_SDK_VERSION_STRING
#define PICO_SDK_VERSION_STRING "unknown"
#endif

struct info_t
{
    uint32_t version;
    char name[13];
    uint8_t revision;
};

void device_info(void);
