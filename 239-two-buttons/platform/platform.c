#include "platform.h"
#include "pico/stdlib.h"

#define LED_PIN 25
#define BUTTON_PIN 10

const uint32_t button_pin[] = {10, 9};

void platform_init(void)
{
    stdio_init_all();

    for (uint32_t btn = 0; btn < 2; btn++)
    {
        gpio_init(button_pin[btn]);
        gpio_set_dir(button_pin[btn], GPIO_IN);
        gpio_pull_up(button_pin[btn]);
    }

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    platform_led_set(true);
}

void platform_led_set(bool on)
{
    gpio_put(LED_PIN, on);
}

bool platform_led_get(void)
{
    return gpio_get(LED_PIN);
}

bool platform_button_read(uint32_t btn)
{
    return !gpio_get(button_pin[btn]);
}