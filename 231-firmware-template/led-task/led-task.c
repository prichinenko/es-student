#include "led-task.h"

#include "pico/stdlib.h"
#include <stdio.h>

// Состояние задачи: внутреннее связывание, только для этого файла
static led_state_t led_state;
static uint64_t half_period_us;
static uint64_t last_toggle_us;

// Вспомогательная функция: из других файлов её не вызвать
static void led_set(bool on)
{
    gpio_put(PICO_DEFAULT_LED_PIN, on);
}

static bool led_get(void)
{
    return (bool)gpio_get(PICO_DEFAULT_LED_PIN);
}

bool period_elapsed(uint64_t *last_us, uint64_t period_us)
{
    uint64_t now_us = time_us_64();
    if ((now_us - *last_us) >= period_us)
    {
        while (*last_us + period_us < now_us)
        {
            *last_us += period_us;
        }
        return true;
    }
    return false;
}

bool led_task_set_period_ms(uint32_t period_ms)
{
    // С нулевым полупериодом светодиод переключался бы
    // на каждой итерации: такой период не принимается
    if (period_ms == 0)
    {
        return false;
    }

    half_period_us = (uint64_t)period_ms * 1000 >> 1;
    return true;
}

uint32_t led_task_get_period_ms()
{
    return (half_period_us << 1) * 1000;
}

void led_task_init(void)
{
    // настраиваем вывод светодиода, период 1000 мс, состояние «мигает»
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    led_task_set_period_ms(1000);
    led_task_set_state(LED_STATE_BLINK);
}

void led_task_set_state(led_state_t state)
{
    // запоминаем состояние и выполняем действие при входе:
    // «выключен» — гасим, «горит» — зажигаем, «мигает» — первое переключение сразу
    led_state = state;
    led_set(state == LED_STATE_OFF ? false : true);
    last_toggle_us = time_us_64();
}

led_state_t led_task_get_state(void)
{
    return led_state;
}

void led_task_handle(void)
{
    // «выключен» и «горит» — ничего не делаем, светодиод установлен при входе
    // «мигает» — каждые полпериода переключаем светодиод

    switch (led_state)
    {
    case LED_STATE_BLINK:
        // printf("led_task_handle (%llu)\n", (unsigned long long)(now_us - last_toggle_us));
        if (period_elapsed(&last_toggle_us, half_period_us))
        {
            led_set(!led_get());
        }
        break;

    default:
        break;
    }
}