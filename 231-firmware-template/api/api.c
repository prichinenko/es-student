#include "api.h"
#include "device.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include "profiling.h"
#include "pico/time.h"

typedef void (*api_callback_t)(const command_t *command);

typedef struct
{
    const char *name;
    api_callback_t callback;
    const char *help;
} api_command_t;

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

void cmd_help(const command_t *command);

const static api_command_t api_commands[] = {
    {"help", cmd_help, "list of commands"},
    {"info", cmd_info, "device passport"},
    {"uptime", cmd_uptime, "time since reset"},
    {"calc_pi", cmd_calc_pi, "calculate pi: calc_pi [terms]"},
    {"main_time_exec", cmd_main_time_exec, "superloop iteration: average and maximum"},
    {"main_time_reset", cmd_main_time_reset, "superloop iteration: average and maximum"},
};

const static uint32_t command_count = sizeof(api_commands) / sizeof(api_commands[0]);

void cmd_help(const command_t *command)
{
    for (uint32_t i = 0; i < command_count; i++)
    {
        printf("%-16s %s\n", api_commands[i].name, api_commands[i].help);
    }
}

void api_handle(const command_t *command)
{
    if (command->truncated)
    {
        printf("error: command longer than 63 characters or 4 arguments\n");
        return;
    }

    for (uint32_t i = 0; i < command_count; i++)
    {
        if (strcmp(command->name, api_commands[i].name) == 0)
        {
            if (api_commands[i].callback != NULL)
            {
                api_commands[i].callback(command);
            }

            return;
        }
    }
    printf("error: unknown command '%s', try help\n", command->name);
}