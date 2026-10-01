#include <stdio.h>
#include "clock.h"
#include "log.h"
#include "pico/stdlib.h"
#include "hardware/clocks.h"

const uint32_t CLK_SYS_LOW_KHZ = 62500;

static void row(const char *name, uint32_t set_khz, uint32_t measured_khz)
{
    printf("%-8s %9u %12u\n", name, (unsigned)set_khz, (unsigned)measured_khz);
}

void uptime(void)
{
    printf("uptime: %llu ms\n", time_us_64() / 1000);
}

void clk_info(void)
{
    printf("%-8s %-9s %-12s\n", "signal", "set_khz", "measured_khz");

    // uint32_t sys_khz = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    row("clk_ref", clock_get_hz(clk_ref) / 1000, frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_REF));
    row("clk_sys", clock_get_hz(clk_sys) / 1000, frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS));
    row("clk_peri", clock_get_hz(clk_peri) / 1000, frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI));
    row("clk_usb", clock_get_hz(clk_usb) / 1000, frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB));
    row("clk_adc", clock_get_hz(clk_adc) / 1000, frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_ADC));
    row("rosc", 0, frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC));
}

static void clk_sys_set(uint32_t khz)
{
    if (set_sys_clock_khz(khz, false))
    {
        LOG_INF("clk_sys %u kHz\n", (unsigned)khz);
    }
    else
    {
        LOG_ERR("clk_sys %u kHz is not set\n", (unsigned)khz);
    }
}

void clk_sys_low(void)
{
    clk_sys_set(CLK_SYS_LOW_KHZ);
}

void clk_sys_default(void)
{
    clk_sys_set(SYS_CLK_KHZ);
}