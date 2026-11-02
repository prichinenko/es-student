#include "platform.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"

#define LED_PIN 25
#define BUTTON_PIN 10

#define CHARGE_PIN 22

// вход АЦП: вывод GP26, канал 0
#define ADC_PIN 26
#define ADC_CHANNEL 0

// опорное напряжение АЦП на плате Pico и число уровней 12-разрядного кода
#define ADC_VREF_V 3.3f
#define ADC_LEVELS 4096

// встроенный датчик температуры — канал 4
#define ADC_TEMP_CHANNEL 4

void platform_init(void)
{
    stdio_init_all();

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    platform_led_set(true);

    gpio_init(CHARGE_PIN);
    gpio_set_dir(CHARGE_PIN, GPIO_OUT);
    platform_charge_set(false);

    adc_init();
    adc_gpio_init(ADC_PIN);

    adc_set_temp_sensor_enabled(true);
}

void platform_led_set(bool on)
{
    gpio_put(LED_PIN, on);
}

bool platform_led_get(void)
{
    return !gpio_get(LED_PIN);
}

bool platform_button_read(void)
{
    return !gpio_get(BUTTON_PIN);
}

float platform_voltage_read(void)
{
    adc_select_input(ADC_CHANNEL);
    return adc_read() * ADC_VREF_V / ADC_LEVELS;
}

// температура кристалла по встроенному датчику, градусы Цельсия
float platform_temp_read(void)
{
    adc_select_input(ADC_TEMP_CHANNEL);
    return 27 - (adc_read() * ADC_VREF_V / ADC_LEVELS - 0.706) / 0.001721;
}

void platform_charge_set(bool on)
{
    gpio_put(CHARGE_PIN, on);
}
