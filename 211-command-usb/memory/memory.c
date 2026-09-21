#include "memory.h"
#include <stdio.h>
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"
#include "hardware/regs/addressmap.h"

extern char __flash_binary_start;
extern char __flash_binary_end;
extern char __boot2_start__;
extern char __boot2_end__;
extern char __etext;
extern char __data_start__;
extern char __data_end__;
extern char __bss_start__;
extern char __bss_end__;
extern char __HeapLimit;
extern char __StackBottom;
extern char __StackTop;

static void row(const char *name, uintptr_t start, uintptr_t end)
{
    printf("%-10s 0x%08x 0x%08x %8u\n",
           name, (unsigned)start, (unsigned)end, (unsigned)(end - start));
}

// Реальный размер flash — спрашиваем чип командой JEDEC ID (0x9F).
// Возвращает 0, если ответ не похож на корректный.
static uint32_t flash_size_real(void)
{
    uint8_t tx[4] = {0x9F, 0x00, 0x00, 0x00}; // команда + 3 dummy-байта
    uint8_t rx[4] = {0};

    uint32_t save = save_and_disable_interrupts();
    flash_do_cmd(tx, rx, sizeof(tx));
    restore_interrupts(save);

    uint8_t mfr = rx[1];  // производитель
    uint8_t type = rx[2]; // тип памяти
    uint8_t cap = rx[3];  // log2(размер в байтах)

    printf("JEDEC: %02X %02X %02X %02X\n", rx[0], rx[1], rx[2], rx[3]);

    // Санити-чек: производитель не должен быть 0x00/0xFF,
    // capacity для реальных чипов лежит в диапазоне 0x14..0x20 (1 МиБ..1 ГиБ).
    if (mfr == 0x00 || mfr == 0xFF)
        return 0;
    if (cap < 0x14 || cap > 0x20)
        return 0;

    (void)type;
    return 1u << cap;
}

static const char *flash_vendor(uint8_t id)
{
    switch (id)
    {
    case 0xEF:
        return "Winbond";
    case 0xC2:
        return "Macronix";
    case 0x1F:
        return "Atmel/Adesto";
    case 0x20:
        return "Micron";
    case 0x01:
        return "Spansion";
    case 0xBF:
        return "SST";
    case 0x0B:
        return "XTX";
    case 0x5E:
        return "Zbit";
    case 0x85:
        return "Puya";
    default:
        return "unknown";
    }
}

void mem_info(void)
{
    printf("area\tstart\tend\tsize\n");
    row("flash", XIP_BASE, XIP_BASE + PICO_FLASH_SIZE_BYTES);
    row("sram", SRAM_BASE, SRAM_END);
    row("rom", ROM_BASE, ROM_BASE + 16384);
    row("image", (uintptr_t)&__flash_binary_start, (uintptr_t)&__flash_binary_end);
    row("free", (uintptr_t)&__flash_binary_end, XIP_BASE + PICO_FLASH_SIZE_BYTES);
    row("boot2", (uintptr_t)&__boot2_start__, (uintptr_t)&__boot2_end__);
    row("text", (uintptr_t)&__boot2_end__, (uintptr_t)&__etext);
    row("data flash", (uintptr_t)&__etext, (uintptr_t)&__etext + ((uintptr_t)&__data_end__ - (uintptr_t)&__data_start__));
    row("data ram", (uintptr_t)&__data_start__, (uintptr_t)&__data_end__);
    row("bss", (uintptr_t)&__bss_start__, (uintptr_t)&__bss_end__);
    row("heap", (uintptr_t)&__bss_end__, (uintptr_t)&__HeapLimit);
    row("stack", (uintptr_t)&__StackBottom, (uintptr_t)&__StackTop);

    printf("\ntotal\n");
    printf(
        "  flash image\t%8u = boot2 %u + text + %u + data %u\n",
        (uintptr_t)&__boot2_end__ - (uintptr_t)&__boot2_start__ + (uintptr_t)&__etext - (uintptr_t)&__boot2_end__ + (uintptr_t)&__data_end__ - (uintptr_t)&__data_start__,
        (uintptr_t)&__boot2_end__ - (uintptr_t)&__boot2_start__,
        (uintptr_t)&__etext - (uintptr_t)&__boot2_end__,
        (uintptr_t)&__data_end__ - (uintptr_t)&__data_start__);
    printf(
        "  flash free\t%8u of %u\n",
        XIP_BASE + PICO_FLASH_SIZE_BYTES - (uintptr_t)&__flash_binary_end,
        PICO_FLASH_SIZE_BYTES);
    printf(
        "  ram used\t%8u = data %u + bss 3508\n",
        (uintptr_t)&__data_end__ - (uintptr_t)&__data_start__ + (uintptr_t)&__bss_end__ - (uintptr_t)&__bss_start__,
        (uintptr_t)&__data_end__ - (uintptr_t)&__data_start__,
        (uintptr_t)&__bss_end__ - (uintptr_t)&__bss_start__);
    printf(
        "  ram free\t%8u for heap and %u for stack\n",
        (uintptr_t)&__HeapLimit - (uintptr_t)&__bss_end__,
        (uintptr_t)&__StackTop - (uintptr_t)&__StackBottom);

    printf("\n\tflash size\t%10u\n", PICO_FLASH_SIZE_BYTES);

    // Реальный размер — спрашиваем чип
    uint8_t tx[1] = {0x9F};
    uint8_t rx[3] = {0};
    uint32_t save = save_and_disable_interrupts();
    flash_do_cmd(tx, rx, sizeof(rx));
    restore_interrupts(save);

    printf("\nchip vendor      %s (0x%02X)\n", flash_vendor(rx[0]), rx[0]);
    printf("\ncmake said       %u bytes (PICO_FLASH_SIZE_BYTES)\n", PICO_FLASH_SIZE_BYTES);
    uint32_t real = flash_size_real();
    if (real == 0)
    {
        printf("chip flash size  <no valid JEDEC response: %02x %02x %02x>\n",
               rx[0], rx[1], rx[2]);
    }
    else
    {
        printf("chip flash size  %u bytes (2^%u)\n", real, rx[2]);
    }
}