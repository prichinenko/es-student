#include "main.h"
#include "led.h"
#include "log.h"
#include "device.h"
#include "memory.h"
#include <stdio.h>
#include <string.h>
#include "command.h"

#define LINE_SIZE 32

// typedef void (*command_handler_t)(void);
// struct command_t
// {
//     const char *name;
//     command_handler_t handler;
// };

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

void cmd_enable(void)
{
    led_set(true);
    LOG_INF("led %s\n", led_is_on() ? "on" : "off");
}

void cmd_disable(void)
{
    led_set(false);
    LOG_INF("led %s\n", led_is_on() ? "on" : "off");
}

void cmd_info(void)
{
    device_info();
}

void cmd_version(void)
{
    log_version();
}

void cmd_ping(void)
{
    printf("pong\n");
}

void cmd_mem_info(void)
{
    mem_info();
}

void cmd_fw_info(void)
{
    fw_info();
}

void cmd_dev_info(void)
{
    dev_info();
}

const struct command_t commands[] = {
    {"enable", cmd_enable},
    {"disable", cmd_disable},
    {"info", cmd_info},
    {"version", cmd_version},
    {"ping", cmd_ping},
    {"mem_info", cmd_mem_info},
    {"fw_info", cmd_fw_info},
    {"dev_info", cmd_dev_info},
};
const uint command_count = sizeof(commands) / sizeof(commands[0]);

void handle_command(const char *command)
{
    for (uint i = 0; i < command_count; i++)
    {
        if (strcmp(command, commands[i].name) == 0)
        {
            if (commands[i].handler != NULL)
            {
                commands[i].handler();
            }

            return;
        }
    }

    LOG_ERR("unknown command: %s\n", command);
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
