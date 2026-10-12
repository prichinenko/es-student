#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef void (*button_callback_t)(void);

void button_task_init(uint32_t btn, button_callback_t callback_button_press, button_callback_t callback_button_log_press);
void button_task_handle(void);
bool button_task_is_pressed(uint32_t btn);
uint32_t button_task_get_press_count(uint32_t btn);