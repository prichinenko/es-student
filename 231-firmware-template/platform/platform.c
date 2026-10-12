#include "platform.h"
#include "pico/stdlib.h"

#define LED_PIN 25
#define BUTTON_PIN 10

void platform_init(void)
{
    stdio_init_all();

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

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
    return !gpio_get(LED_PIN);
}

bool platform_button_read(void)
{
    return !gpio_get(BUTTON_PIN);
}