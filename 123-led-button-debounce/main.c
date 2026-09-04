#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/regs/addressmap.h"
#include "hardware/regs/sio.h"

const uint8_t LED_PIN = 25;
const uint8_t BUTTON_PIN = 24;
const uint8_t DEBUONCE_MS = 20;

bool get_button_debuonce(uint8_t pin)
{
    bool state = gpio_get(pin);
    sleep_ms(DEBUONCE_MS);
    return state && gpio_get(pin);
}

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
        bool current = get_button_debuonce(BUTTON_PIN);

        if (previous && !current)
        {
            led = !led;
            gpio_put(LED_PIN, led);
        }

        previous = current;
    }

    return 0;
}
