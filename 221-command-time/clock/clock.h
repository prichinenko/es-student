#pragma once
#include "pico/stdlib.h"

void clk_info(void);
void uptime(void);
void clk_sys_low(void);
void clk_sys_default(void);
void clk_sys_overclock(uint32_t khz);