#pragma once

#include <stdint.h>

#define DEVICE_NAME "es-cmd-usb"
#define FIRMWARE_VERSION "1.0.0"

#define DEVICE_PROJECT "211-command-usb"
#define DEVICE_REPO "https://github.com/prichinenko/es-student"

#ifndef DEVICE_BOARD
#define DEVICE_BOARD "unknown"
#endif

#ifndef PICO_SDK_VERSION_STRING
#define PICO_SDK_VERSION_STRING "unknown"
#endif

#define VECTOR_TABLE 0x10000100

struct info_t
{
    uint32_t version;
    char name[13];
    uint8_t revision;
};

extern struct info_t device_card;

void device_info(void);
void dev_info(void);
