#include "api.h"
#include "device/device.h"
#include "systime/systime.h"
#include "led-task.h"
#include "button-task.h"
#include "pi-task.h"
#include "profiling/profiling.h"
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
void cmd_pi_start(const command_t *command);
void cmd_pi(const command_t *command);
void cmd_led_enable(const command_t *command);
void cmd_led_disable(const command_t *command);
void cmd_led_blink(const command_t *command);
void cmd_led_period(const command_t *command);
void cmd_button(const command_t *command);
void cmd_profiling(const command_t *command);
void cmd_profiling_reset(const command_t *command);

const api_command_t api_commands[] = {
    {"help", "list of commands", cmd_help},
    {"info", "device passport", cmd_info},
    {"uptime", "time since reset", cmd_uptime},
    {"led_enable", "led on", cmd_led_enable},
    {"led_disable", "led off", cmd_led_disable},
    {"led_blink", "led blink", cmd_led_blink},
    {"led_period", "led peruid ms: led_period [period]", cmd_led_period},
    {"button", "button status", cmd_button},
    {"pi_start", "start calculate pi: pi_start [terms]", cmd_pi_start},
    {"pi", "pi result", cmd_pi},
    {"profiling", "profiling", cmd_profiling},
    {"profiling_reset", "profiling reset", cmd_profiling_reset},
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
    device_info_t info;
    device_get_info(&info);

    printf("project: %s\n", info.project);
    printf("repo: %s\n", info.repo);
    printf("board: %s\n", info.board);
    printf("serial: %s\n", info.serial);
    printf(
        "chip: manufacturer 0x%03x, part 0x%04x, revision %u\n",
        (unsigned int)info.chip_manufacturer,
        (unsigned int)info.chip_part,
        (unsigned int)info.chip_revision);
    printf("pico-sdk: %s\n", info.pico_sdk);
}

void cmd_clk_sys_overclock(const command_t *command)
{
    // clk_sys_overclock(250000);
}

void cmd_uptime(const command_t *command)
{
    printf("uptime: %llu ms\n", (long long unsigned int)systime_ms());
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
    uint64_t start_us = systime_us();
    double pi_result = calc_pi(terms);
    uint64_t spent_us = systime_us() - start_us;

    printf("pi: %.8f\n", pi_result);
    printf("time: %llu ms\n", spent_us / 1000);
}

void cmd_pi_start(const command_t *command)
{
    uint32_t terms = 1000000;
    if (command->argc > 0)
    {
        parse_u32(command->argv[0], &terms);
    }

    if (terms <= 0)
    {
        printf("error: usage calc_pi [terms]\n");
        return;
    }

    if (pi_task_start(terms))
    {
        printf("pi: started, %u terms\n", (unsigned int)terms);
    }
    else
    {
        printf("error: pi is busy\n");
    }
}

void cmd_pi(const command_t *command)
{
    pi_task_result_t result = pi_task_get_result();
    switch (result.state)
    {
    case PI_STATE_NOT_STARTED:
        printf("pi: not_started\n");
        break;

    case PI_STATE_PROCESS:
        printf("pi: runing\n");
        break;

    case PI_STATE_FINISH:
        printf("pi: %.8f in %llu ms\n", result.pi, result.time / 1000);
        break;
    }
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
        "button: %s, presses %u\n",
        button_task_is_pressed() ? "pressed" : "released",
        (unsigned int)button_task_get_press_count());
}

void cmd_profiling(const command_t *command)
{
#if PROFILING_ENABLED
    for (uint32_t id = 0; id < STOPWATCH_COUNT; ++id)
    {
        profiling_result_t result;
        profiling_get(id, &result);
        printf(
            "%-18s\tcount %u\tmean %f us\tmax %u us\n",
            result.name,
            (unsigned int)result.n,
            result.avg_us,
            (unsigned int)result.max_us);
    }
#else
    printf("profiling is disabled in this build\n");
#endif
}

void cmd_profiling_reset(const command_t *command)
{
    profiling_reset();
}
