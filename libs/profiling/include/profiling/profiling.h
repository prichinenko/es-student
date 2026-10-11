#pragma once

#include <stdint.h>
#include <stdbool.h>

// по умолчанию профилирование включено
#ifndef PROFILING_ENABLED
#define PROFILING_ENABLED 1
#endif

typedef enum
{
    STOPWATCH_LOOP,
    STOPWATCH_COMMAND,
    STOPWATCH_LED,
    STOPWATCH_BUTTON,
    STOPWATCH_PI,
    STOPWATCH_COUNT
} profiling_stopwatch_t;

// сумма замеров одного секундомера переполнится только через 71 минуту
typedef struct
{
    char *name;
    uint32_t sum;
    uint32_t n;
    uint32_t start_us;
    uint32_t max_us;
    double avg_us;
} profiling_result_t;

#if PROFILING_ENABLED
void profiling_start(uint32_t id);
void profiling_stop(uint32_t id);
#else
// профилирование выключено: вызовы ничего не делают
#define profiling_start(id) ((void)0)
#define profiling_stop(id) ((void)0)
#endif

void profiling_stopwatch_init(uint32_t id, const char *name);

// false — секундомер с таким номером не заведён
bool profiling_get(uint32_t id, profiling_result_t *result);
void profiling_reset(void);