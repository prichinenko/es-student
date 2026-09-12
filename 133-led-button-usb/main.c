#include "main.h"
#include <stdio.h>

const uint8_t LED_PIN = 25;
const uint8_t BUTTON_PIN = 24;
const uint8_t DEBUONCE_MS = 20;

bool get_button_debounce(uint pin)
{
    bool state = gpio_get(pin);
    sleep_ms(DEBUONCE_MS);
    return state && gpio_get(pin);
}

void set_led(bool on)
{
    gpio_put(LED_PIN, on);
    printf("led %s\n", on ? "on" : "off");
}

int main()
{
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    bool led = false;
    bool previous = false;

    gpio_put(LED_PIN, led);

    while (1)
    {
        // printf("Hello, world!\n");
        // sleep_ms(1000);

        bool current = get_button_debounce(BUTTON_PIN);

        if (previous && !current)
        {
            led = !led;
            set_led(led);
            // gpio_put(LED_PIN, led);
        }

        previous = current;
    }

    return 0;
}
