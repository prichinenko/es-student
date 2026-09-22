#include "device.h"
#include "led.h"
#include <stdio.h>
#include <stddef.h>
#include "pico/unique_id.h"
#include "hardware/regs/addressmap.h"
#include "hardware/regs/sysinfo.h"

struct info_t device_card = {.version = 0x00010000, .revision = 2, .name = DEVICE_NAME};

void device_info(void)
{
    char board_id[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1];
    pico_get_unique_board_id_string(board_id, sizeof(board_id));

    volatile uint32_t *chip_id = (uint32_t *)(SYSINFO_BASE + SYSINFO_CHIP_ID_OFFSET);
    uint32_t id = *chip_id;

    uint32_t manufacturer = (id & SYSINFO_CHIP_ID_MANUFACTURER_BITS) >> SYSINFO_CHIP_ID_MANUFACTURER_LSB;
    uint32_t part = (id & SYSINFO_CHIP_ID_PART_BITS) >> SYSINFO_CHIP_ID_PART_LSB;
    uint32_t revision = (id & SYSINFO_CHIP_ID_REVISION_BITS) >> SYSINFO_CHIP_ID_REVISION_LSB;

    printf("project: %s\n", DEVICE_PROJECT);
    printf("repo: %s\n", DEVICE_REPO);
    printf("board: %s\n", DEVICE_BOARD);
    printf("serial: %s\n", board_id);
    printf("chip: manufacturer 0x%03x, part 0x%04x, revision %u\n", manufacturer, part, revision);
    printf("pico-sdk: %s\n", PICO_SDK_VERSION_STRING);
}

void dev_info(void)
{
    printf("%-15s %-10s %-5s %-6s %s\n",
           "struct", "address", "size", "offset", "value");
    printf("%-15s 0x%08x %5u\n",
           "device_card",
           &device_card,
           sizeof(device_card));
    printf("- %-13s 0x%08x %5u %6u 0x%08x\n",
           "version",
           &device_card.version,
           sizeof(device_card.version),
           offsetof(struct info_t, version),
           device_card.version);
    printf("- %-13s 0x%08x %5u %6u %s\n",
           "name",
           &device_card.name,
           sizeof(device_card.name),
           offsetof(struct info_t, name),
           device_card.name);
    printf("- %-13s 0x%08x %5u %6u %u\n",
           "revision",
           &device_card.revision,
           sizeof(device_card.revision),
           offsetof(struct info_t, revision),
           device_card.revision);

    unsigned fields = sizeof(device_card.version) + sizeof(device_card.name) + sizeof(device_card.revision);
    printf("fields %u, sizeof %u, padding %u\n", fields, sizeof(device_card), sizeof(device_card) - fields);
}

void boot_info(void)
{
    const uint32_t *vectors = (const uint32_t *)VECTOR_TABLE;

    uint32_t stack_top = vectors[0];
    uint32_t reset_handler = vectors[1];

    volatile uint32_t *gpio_in = (uint32_t *)(SIO_BASE + SIO_GPIO_IN_OFFSET);
    uint32_t level = (*gpio_in >> led_pin()) & 1u;

    printf("vector table\t0x%08x\n", VECTOR_TABLE);
    printf("  stack top\t0x%08x\n", stack_top);
    printf("  reset    \t0x%08x\n", reset_handler);
    printf("  reset (even)\t0x%08x\n", reset_handler & ~1u);
    printf("gpio in  \t0x%08x\n", gpio_in);
    printf("  led bit\t%u\n", level);
    printf("  gpio_get\t%u\n", gpio_get(led_pin()));
}
