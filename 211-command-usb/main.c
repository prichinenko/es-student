#include "main.h"
#include "led.h"
#include "log.h"
#include "device.h"
#include <stdio.h>
#include <string.h>

#define LINE_SIZE 32

const uint8_t BUTTON_PIN = 24;
const uint8_t DEBUONCE_MS = 20;

char line[LINE_SIZE];
uint line_length = 0;

bool get_button_debounce(uint pin)
{
    bool state = gpio_get(pin);
    sleep_ms(DEBUONCE_MS);
    return state && gpio_get(pin);
}

void read_line(void)
{
    int symbol = getchar_timeout_us(0);

    if (symbol == PICO_ERROR_TIMEOUT)
    {
        return;
    }

    if (symbol == '\r' || symbol == '\n')
    {
        putchar('\n');
        line[line_length] = '\0';

        if (line_length > 0)
        {
            LOG_DBG("got %s\n", line);
            handle_command(line);
        }

        line_length = 0;
        return;
    }

    if (line_length + 1 < LINE_SIZE)
    {
        line[line_length] = (char)symbol;
        line_length = line_length + 1;
        putchar(symbol);
    }
}

void handle_command(const char *command)
{
    if (strcmp(command, "enable") == 0)
    {
        led_set(true);
        LOG_INF("led %s\n", led_is_on() ? "on" : "off");
    }
    else if (strcmp(command, "disable") == 0)
    {
        led_set(false);
        LOG_INF("led %s\n", led_is_on() ? "on" : "off");
    }
    else if (strcmp(command, "info") == 0)
    {
        device_info();
    }
    else if (strcmp(command, "version") == 0)
    {
        log_version();
    }
    else
    {
        LOG_ERR("unknown command: %s\n", command);
    }
}

int main()
{
    stdio_init_all();

    led_init();
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    bool previous = false;

    while (1)
    {
        bool current = get_button_debounce(BUTTON_PIN);

        if (previous && !current)
        {
            led_toggle();
            LOG_INF("led %s\n", led_is_on() ? "on" : "off");
        }

        previous = current;

        read_line();

        // int command = getchar_timeout_us(0);

        // if (command == PICO_ERROR_TIMEOUT)
        // {
        //     continue;
        // }

        // LOG_DBG("got %c\n", command);
        // handle_command(command);
    }

    return 0;
}
