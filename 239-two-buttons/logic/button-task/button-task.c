#include "button-task.h"
#include "systime/systime.h"
#include "logging/log.h"
#include "platform.h"

#include <stdio.h>

typedef enum
{
    BUTTON_STATE_RELEASED,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_RELEASE,
    BUTTON_STATE_PRESS,
} button_state_t;

typedef struct
{
    button_state_t last_state;
    uint64_t last_toggle_us;
    uint32_t debuonce_us;
    uint32_t press_count;
    void (*button_task_callback_button_press)(void);
    void (*button_task_callback_button_long_press)(void);
    bool is_long_press;
} button_item_t;

static button_item_t buttons[2];

static const uint32_t long_press_us = 1000000;

static button_state_t get_state(uint32_t btn)
{
    return platform_button_read(btn) ? BUTTON_STATE_PRESSED : BUTTON_STATE_RELEASED;
}

void button_task_init(uint32_t btn, void (*callback_button_press)(void), void (*callback_button_log_press)(void))
{
    buttons[btn].last_state = get_state(btn);
    buttons[btn].last_toggle_us = systime_us();
    buttons[btn].press_count = 0;
    buttons[btn].debuonce_us = 20000;
    buttons[btn].button_task_callback_button_press = callback_button_press;
    buttons[btn].button_task_callback_button_long_press = callback_button_log_press;
    buttons[btn].is_long_press = false;
}

void button_task_handle(void)
{
    for (uint32_t btn = 0; btn < 2; btn++)
    {
        button_state_t button_state_now = get_state(btn);
        uint64_t now = systime_us();

        // if (buttons[btn].last_state == button_state_now)
        // {
        //     continue;
        // }

        // printf("btn: %u (%u)\n", button_state_now, last_state);

        switch (buttons[btn].last_state)
        {
        case BUTTON_STATE_PRESSED:
            if (button_state_now == BUTTON_STATE_RELEASED)
            {
                buttons[btn].last_state = BUTTON_STATE_RELEASE;
                buttons[btn].last_toggle_us = now;
                if (!buttons[btn].is_long_press && buttons[btn].button_task_callback_button_press)
                {
                    buttons[btn].button_task_callback_button_press();
                }
                buttons[btn].is_long_press = false;
            }
            else if (
                button_state_now == BUTTON_STATE_PRESSED && !buttons[btn].is_long_press && (now - buttons[btn].last_toggle_us) > long_press_us)
            {
                buttons[btn].is_long_press = true;
                if (buttons[btn].button_task_callback_button_long_press)
                {
                    buttons[btn].button_task_callback_button_long_press();
                }
            }
            break;

        case BUTTON_STATE_RELEASED:
            if (button_state_now == BUTTON_STATE_PRESSED)
            {
                buttons[btn].last_state = BUTTON_STATE_PRESS;
                buttons[btn].last_toggle_us = now;
            }
            break;

        case BUTTON_STATE_PRESS:
            buttons[btn].is_long_press = false;
            // LOG_INF("BUTTON_STATE_PRESS\n");
            if (button_state_now == BUTTON_STATE_RELEASED)
            {
                buttons[btn].last_state = BUTTON_STATE_RELEASED;
            }
            else
            {
                // printf("\tdeuonce on press %u\n", (unsigned int)(now - last_toggle_us));
                if (now - buttons[btn].last_toggle_us > buttons[btn].debuonce_us)
                {
                    buttons[btn].last_state = BUTTON_STATE_PRESSED;
                    buttons[btn].press_count++;
                }
            }
            break;

        case BUTTON_STATE_RELEASE:
            buttons[btn].is_long_press = false;
            if (button_state_now == BUTTON_STATE_PRESSED)
            {
                buttons[btn].last_state = BUTTON_STATE_PRESSED;
            }
            else
            {
                if (now - buttons[btn].last_toggle_us > buttons[btn].debuonce_us)
                {
                    buttons[btn].last_state = BUTTON_STATE_RELEASED;
                }
            }
            break;
        }
    }
}

bool button_task_is_pressed(uint32_t btn)
{
    return buttons[btn].last_state == BUTTON_STATE_PRESSED;
}

uint32_t button_task_get_press_count(uint32_t btn)
{
    return buttons[btn].press_count;
}