#include "main.h"
#include "led.h"
#include "log.h"
#include "device.h"
#include "led-task.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "command.h"
#include "profiling.h"
#include "stdio-text-protocol.h"
#include "api.h"

const uint BLINK_HALF_PERIOD_MS = 500;
uint64_t last_toggle_us = 0;

// прикидка: за член ряда 4 операции с double, 175 + 110 + 190 + 110 = 585 тактов;
// 1 000 000 членов по 585 тактов при 125 МГц — около 4,7 с
const uint CALC_PI_TERMS = 1000000;

double calc_pi(uint terms)
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

// void cmd_calc_pi(void)
// {
//     uint64_t start_us = time_us_64();
//     pi_result = calc_pi(CALC_PI_TERMS);
//     uint64_t spent_us = time_us_64() - start_us;

//     printf("pi: %.8f\n", pi_result);
//     printf("time: %llu ms\n", spent_us / 1000);
// }

void blink(void)
{
    uint64_t now_us = time_us_64();

    if (now_us - last_toggle_us >= BLINK_HALF_PERIOD_MS * 1000)
    {
        last_toggle_us = now_us;
        led_toggle();
    }
}

void core1_entry()
{
    // Инициализация, специфичная для Core 1
    while (1)
    {
        // Основной цикл Core 1
        led_task_handle();
    }
}

int main()
{
    stdio_init_all();
    led_task_init();

    profiling_init();

    // Запускаем Core 1. Он начнёт выполнять core1_entry()
    multicore_launch_core1(core1_entry);

    while (1)
    {
        // led_task_handle();

        // принятая строка становится командой, команда — действием прибора
        const command_t *command = stdio_text_protocol_handle();
        if (command != NULL)
        {
            api_handle(command);
        }

        profiling_iteration();
    }

    return 0;
}
