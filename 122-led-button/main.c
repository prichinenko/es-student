#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/regs/addressmap.h"
#include "hardware/regs/sio.h"

const uint LED_PIN = 25;
const uint8_t BUTTON_PIN = 24;

int main()
{
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
        bool current = gpio_get(BUTTON_PIN);

        if (previous && !current)
        {
            led = !led;
            gpio_put(LED_PIN, led);
        }

        previous = current;
    }

    return 0;
}
