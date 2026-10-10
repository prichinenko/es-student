#include "button-task.h"
#include "pico/stdlib.h"
#include <stdio.h>

typedef enum
{
    BUTTON_STATE_RELEASED,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_RELEASE,
    BUTTON_STATE_PRESS,
} button_state_t;

button_state_t button_state;

static button_state_t last_state;
static uint64_t last_toggle_us;
static uint32_t debuonce_us;
static uint32_t press_count;

static button_state_t get_state(void)
{
    return gpio_get(BUTTON_PIN) ? BUTTON_STATE_RELEASED : BUTTON_STATE_PRESSED;
}

void button_task_init(void)
{
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
    last_state = get_state();
    last_toggle_us = time_us_64();
    press_count = 0;
    debuonce_us = 20000;
}

void button_task_handle(void)
{
    button_state_t button_state_now = get_state();
    uint64_t now = time_us_64();

    if (last_state == button_state_now)
    {
        return;
    }

    // printf("btn: %u (%u)\n", button_state_now, last_state);

    switch (last_state)
    {
    case BUTTON_STATE_PRESSED:
        if (button_state_now == BUTTON_STATE_RELEASED)
        {
            last_state = BUTTON_STATE_RELEASE;
            last_toggle_us = now;
        }
        break;

    case BUTTON_STATE_RELEASED:
        if (button_state_now == BUTTON_STATE_PRESSED)
        {
            last_state = BUTTON_STATE_PRESS;
            last_toggle_us = now;
        }
        break;

    case BUTTON_STATE_PRESS:
        if (button_state_now == BUTTON_STATE_RELEASED)
        {
            last_state = BUTTON_STATE_RELEASED;
        }
        else
        {
            // printf("\tdeuonce on press %u\n", (unsigned int)(now - last_toggle_us));
            if (now - last_toggle_us > debuonce_us)
            {
                last_state = BUTTON_STATE_PRESSED;
                press_count++;
            }
        }
        break;

    case BUTTON_STATE_RELEASE:
        if (button_state_now == BUTTON_STATE_PRESSED)
        {
            last_state = BUTTON_STATE_PRESSED;
        }
        else
        {
            if (now - last_toggle_us > debuonce_us)
            {
                last_state = BUTTON_STATE_RELEASED;
            }
        }
        break;
    }
}

bool button_task_is_pressed(void)
{
    return last_state == BUTTON_STATE_PRESSED;
}

uint32_t button_task_get_press_count(void)
{
    return press_count;
}