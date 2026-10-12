#include "main.h"
#include "firmware.h"
#include "logging/logging.h"
#include "device/device.h"
#include "led-task/led-task.h"
#include "button-task/button-task.h"
#include "pi-task/pi-task.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "platform.h"
#include "profiling/profiling.h"
#include "systime/systime.h"
#include "stdio-text-protocol/stdio-text-protocol.h"
#include "api/api.h"

typedef enum
{
    LOOP
} profiling_result_main_t;

const uint BLINK_HALF_PERIOD_MS = 500;
uint64_t last_toggle_us = 0;

// прикидка: за член ряда 4 операции с double, 175 + 110 + 190 + 110 = 585 тактов;
// 1 000 000 членов по 585 тактов при 125 МГц — около 4,7 с
const uint CALC_PI_TERMS = 1000000;

double calc_pi(uint32_t terms)
{
    double sum = 0.0;
    double sign = 1.0;

    for (int k = 0; k < terms; k++)
    {
        // Прибавить очередной член ряда: ±1 / (2k + 1)
        sum += sign / (2.0 * k + 1.0);
        sign = -sign;
    }

    // Сумма ряда равна π/4
    return 4.0 * sum;
}

volatile double pi_result;

void core1_entry()
{
    uint64_t next_time = systime_us() + 500;
    // Инициализация, специфичная для Core 1
    while (1)
    {
        // Основной цикл Core 1

        if (systime_us() > next_time)
        {
            next_time = systime_us() + 500;

            profiling_start(STOPWATCH_LED);
            led_task_handle();
            profiling_stop(STOPWATCH_LED);

            profiling_start(STOPWATCH_BUTTON);
            button_task_handle();
            profiling_stop(STOPWATCH_BUTTON);
        }
    }
}

int main()
{
    platform_init();

    led_task_init();
    button_task_init(led_task_next_state);
    pi_task_init();

    profiling_stopwatch_init(STOPWATCH_LOOP, "loop");
    profiling_stopwatch_init(STOPWATCH_COMMAND, "command");
    profiling_stopwatch_init(STOPWATCH_LED, "led");
    profiling_stopwatch_init(STOPWATCH_BUTTON, "button");
    profiling_stopwatch_init(STOPWATCH_PI, "pi");

    LOG_INF("%s %s\n", FIRMWARE_NAME, FIRMWARE_VERSION);

    // Запускаем Core 1. Он начнёт выполнять core1_entry()
    multicore_launch_core1(core1_entry);

    while (1)
    {
        profiling_start(STOPWATCH_LOOP);

        profiling_start(STOPWATCH_PI);
        pi_task_handle();
        profiling_stop(STOPWATCH_PI);

        profiling_start(STOPWATCH_COMMAND);
        // принятая строка становится командой, команда — действием прибора
        const command_t *command = stdio_text_protocol_handle();
        if (command != NULL)
        {
            api_handle(command);
        }
        profiling_stop(STOPWATCH_COMMAND);

        profiling_stop(STOPWATCH_LOOP);
    }

    return 0;
}
