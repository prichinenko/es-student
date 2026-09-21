// led.h
#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void led_init(void);
    void led_set(bool on);
    void led_toggle(void);
    bool led_is_on(void);

    typedef struct led_s led_t; /* неполный тип: полей нет, только указатель */

    led_t *led_bridge_create(uint pin);
    void led_bridge_destroy(led_t *self);
    void led_bridge_set(led_t *self, bool on);
    void led_bridge_toggle(led_t *self);
    bool led_bridge_is_on(const led_t *self);

#ifdef __cplusplus
}
#endif