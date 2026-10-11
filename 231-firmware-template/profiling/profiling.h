#pragma once

#include "pico/types.h"

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

void profiling_init(void);
void profiling_iteration(void);
float profiling_avg_us(void);
uint32_t profiling_max_us(void);
void profiling_reset_max(void);

void profiling_stopwatch_init(uint32_t id, const char *name);

void profiling_start(uint32_t id);
void profiling_stop(uint32_t id);

// false — секундомер с таким номером не заведён
bool profiling_get(uint32_t id, profiling_result_t *result);
void profiling_reset(void);
