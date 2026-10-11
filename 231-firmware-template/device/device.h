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

typedef struct
{
    char *project;
    char *repo;
    char *board;
    char *serial;
    uint32_t chip_manufacturer;
    uint32_t chip_part;
    uint32_t chip_revision;
    char *pico_sdk;
} device_info_t;

struct info_t
{
    uint32_t version;
    char name[13];
    uint8_t revision;
};

void device_info(void);
void device_get_info(device_info_t *info);