#pragma once
#include <stdint.h>
#include <stdbool.h>

void button_task_init(uint32_t btn, void (*callback_button_press)(void));
void button_task_handle(void);
bool button_task_is_pressed(uint32_t btn);
uint32_t button_task_get_press_count(uint32_t btn);