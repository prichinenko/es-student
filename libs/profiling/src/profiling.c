#include "profiling/profiling.h"
#include "systime/systime.h"

static profiling_result_t profiling_result[STOPWATCH_COUNT];

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
    profiling_result[id].start_us = systime_us();
}

void profiling_start(uint32_t id)
{
    if (id >= STOPWATCH_COUNT)
    {
        return;
    }
    profiling_result[id].start_us = systime_us();
}

void profiling_stop(uint32_t id)
{
    if (id >= STOPWATCH_COUNT)
    {
        return;
    }
    uint32_t stop_us = systime_us();
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