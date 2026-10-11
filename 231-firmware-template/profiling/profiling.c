#include "profiling.h"
#include "pico/stdlib.h"

static uint64_t avg_sum = 0;

static uint32_t previous_us = 0;
static uint32_t max_us = 0;

// итерация около 2 мкс, за секунду их около 500 000;
// ближайшая степень двойки 2^19 = 524 288, среднее помнит около секунды
#define AVG_SHIFT 19
// замер в 1/256 долях микросекунды, чтобы среднее не теряло дробную часть
#define FRACTION_SHIFT 8

static profiling_result_t profiling_result[STOPWATCH_COUNT];

void profiling_init(void)
{
    previous_us = time_us_32();
}

void profiling_iteration(void)
{
    uint32_t now_us = time_us_32();
    uint32_t iteration_us = now_us - previous_us;
    previous_us = now_us;

    if (iteration_us > max_us)
    {
        max_us = iteration_us;
    }

    avg_sum = avg_sum - (avg_sum >> AVG_SHIFT) + ((uint64_t)iteration_us << FRACTION_SHIFT);
}

float profiling_avg_us(void)
{
    return (float)(avg_sum >> AVG_SHIFT) / (1 << FRACTION_SHIFT);
}

void profiling_reset_max(void)
{
    max_us = 0;
}

uint32_t profiling_max_us(void)
{
    return max_us;
}

void profiling_stopwatch_init(uint32_t id, const char *name)
{
    if (id >= STOPWATCH_COUNT)
    {
        return;
    }
    profiling_result[id].name = (char *)name;
    profiling_result[id].n = 0;
    profiling_result[id].sum = 0;
    profiling_result[id].max_us = 0;
    profiling_result[id].start_us = time_us_32();
}

void profiling_start(uint32_t id)
{
    if (id >= STOPWATCH_COUNT)
    {
        return;
    }
    profiling_result[id].start_us = time_us_32();
}

void profiling_stop(uint32_t id)
{
    if (id >= STOPWATCH_COUNT)
    {
        return;
    }
    uint32_t stop_us = time_us_32();
    uint32_t time_us = stop_us - profiling_result[id].start_us;
    profiling_result[id].sum += time_us;
    profiling_result[id].n++;
    profiling_result[id].avg_us = (double)profiling_result[id].sum / profiling_result[id].n;
    if (time_us > profiling_result[id].max_us)
    {
        profiling_result[id].max_us = time_us;
    }
}

// false — секундомер с таким номером не заведён
bool profiling_get(uint32_t id, profiling_result_t *result)
{
    if (id >= STOPWATCH_COUNT)
    {
        return false;
    }
    *result = profiling_result[id];
    return true;
}

void profiling_reset(void)
{
    for (uint32_t id = 0; id < STOPWATCH_COUNT; id++)
    {
        profiling_result[id].n = 0;
        profiling_result[id].sum = 0;
        profiling_result[id].start_us = 0;
        profiling_result[id].avg_us = 0;
        profiling_result[id].max_us = 0;
    }
}