#include "device/device.h"

#include <stdio.h>
#include <stddef.h>
#include "pico/unique_id.h"
#include "hardware/regs/addressmap.h"
#include "hardware/regs/sysinfo.h"

// struct info_t device_card = {.version = 0x00010000, .revision = 2, .name = DEVICE_NAME};

void device_get_info(device_info_t *info)
{
    char board_id[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1];
    pico_get_unique_board_id_string(board_id, sizeof(board_id));

    volatile uint32_t *chip_id = (uint32_t *)(SYSINFO_BASE + SYSINFO_CHIP_ID_OFFSET);
    uint32_t id = *chip_id;

    // info->project = FIRMWARE_PROJECT;
    // info->repo = FIRMWARE_REPO;
    // info->board = FIRMWARE_BOARD;
    info->serial = board_id;
    info->chip_manufacturer = (id & SYSINFO_CHIP_ID_MANUFACTURER_BITS) >> SYSINFO_CHIP_ID_MANUFACTURER_LSB;
    info->chip_part = (id & SYSINFO_CHIP_ID_PART_BITS) >> SYSINFO_CHIP_ID_PART_LSB;
    info->chip_revision = (id & SYSINFO_CHIP_ID_REVISION_BITS) >> SYSINFO_CHIP_ID_REVISION_LSB;
    info->pico_sdk = PICO_SDK_VERSION_STRING;
}
