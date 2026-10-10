#include "api.h"
#include "device.h"
#include "led-task.h"
#include "button-task.h"
#include "profiling.h"
#include "pico/time.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

void cmd_help(const command_t *command);
void cmd_info(const command_t *command);
void cmd_uptime(const command_t *command);
void cmd_calc_pi(const command_t *command);
void cmd_calc_pi(const command_t *command);
void cmd_main_time_exec(const command_t *command);
void cmd_main_time_reset(const command_t *command);
void cmd_led_enable(const command_t *command);
void cmd_led_disable(const command_t *command);
void cmd_led_blink(const command_t *command);
void cmd_led_period(const command_t *command);
void cmd_button(const command_t *command);

const api_command_t api_commands[] = {
    {"help", "list of commands", cmd_help},
    {"info", "device passport", cmd_info},
    {"uptime", "time since reset", cmd_uptime},
    {"calc_pi", "calculate pi: calc_pi [terms]", cmd_calc_pi},
    {"main_time_exec", "superloop iteration: average and maximum", cmd_main_time_exec},
    {"main_time_reset", "superloop iteration: average and maximum", cmd_main_time_reset},
    {"led_enable", "led on", cmd_led_enable},
    {"led_disable", "led off", cmd_led_disable},
    {"led_blink", "led blink", cmd_led_blink},
    {"led_period", "led peruid ms: led_period [period]", cmd_led_period},
    {"button", "button status", cmd_button},
};

const uint32_t api_commands_count = sizeof(api_commands) / sizeof(api_commands[0]);

// Число без знака из десятичной записи. false — в записи не только цифры
// или число не помещается в uint32_t.
static bool parse_u32(const char *text, uint32_t *value)
{
    // strtoul приняла бы и знак: из "-1" получилось бы 4294967295
    if (!isdigit((unsigned char)text[0]))
    {
        return false;
    }

    char *end;
    errno = 0;
    unsigned long number = strtoul(text, &end, 10);

    // после числа остались символы или число не поместилось
    if (*end != '\0' || errno == ERANGE || number > UINT32_MAX)
    {
        return false;
    }

    *value = (uint32_t)number;
    return true;
}

void cmd_info(const command_t *command)
{
    device_info();
}

void cmd_main_time_reset(const command_t *command)
{
    profiling_reset_max();
    printf("max reset\n");
}

void cmd_main_time_exec(const command_t *command)
{
    printf("iteration avg %.2f us, max %u us\n", profiling_avg_us(), (unsigned)profiling_max_us());
}

void cmd_clk_sys_overclock(const command_t *command)
{
    // clk_sys_overclock(250000);
}

void cmd_uptime(const command_t *command)
{
    printf("uptime: %llu ms\n", time_us_64() / 1000);
}

extern double calc_pi(uint terms);
void cmd_calc_pi(const command_t *command)
{
    uint32_t terms = 0;
    if (command->argc != 1 || !parse_u32(command->argv[0], &terms))
    {
        printf("error: usage calc_pi [terms]\n");
        return;
    }
    uint64_t start_us = time_us_64();
    double pi_result = calc_pi(terms);
    uint64_t spent_us = time_us_64() - start_us;

    printf("pi: %.8f\n", pi_result);
    printf("time: %llu ms\n", spent_us / 1000);
}

void cmd_help(const command_t *command)
{
    for (uint32_t i = 0; i < api_commands_count; i++)
    {
        printf("%-16s %s\n", api_commands[i].name, api_commands[i].help);
    }
}

void cmd_led_enable(const command_t *command)
{
    led_task_set_state(LED_STATE_ON);
}

void cmd_led_disable(const command_t *command)
{
    led_task_set_state(LED_STATE_OFF);
}

void cmd_led_blink(const command_t *command)
{
    led_task_set_state(LED_STATE_BLINK);
}

void cmd_led_period(const command_t *command)
{
    uint32_t period = 0;
    if (command->argc != 1 || !parse_u32(command->argv[0], &period))
    {
        printf("error: usage led_period [period]\n");
        return;
    }
    led_task_set_period_ms(period);
}

void cmd_button(const command_t *command)
{
    printf(
        "button: %s, pressed %u\n",
        button_task_is_pressed() ? "pressed" : "released",
        (unsigned int)button_task_get_press_count());
}