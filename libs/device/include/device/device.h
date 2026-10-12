#pragma once

#include <stdint.h>

#ifndef PICO_SDK_VERSION_STRING
#define PICO_SDK_VERSION_STRING "unknown"
#endif

typedef struct
{
    char *name;
    char *version;
    char *project;
    char *repo;
    char *board;
    char *serial;
    uint32_t chip_manufacturer;
    uint32_t chip_part;
    uint32_t chip_revision;
    char *pico_sdk;
} device_info_t;

void device_get_info(device_info_t *info);