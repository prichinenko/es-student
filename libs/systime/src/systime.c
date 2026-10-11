#include "systime/systime.h"
#include "pico/time.h"

uint64_t systime_us(void)
{
    return time_us_64();
}

uint64_t systime_ms(void)
{
    return time_us_64() / 1000;
}

bool systime_period_elapsed(uint64_t *last_us, uint64_t period_us)
{
    uint64_t now_us = time_us_64();
    if ((now_us - *last_us) >= period_us)
    {
        uint64_t correct_last_us = (*last_us += period_us);
        while ((correct_last_us = *last_us + period_us) < now_us)
        {
            *last_us = correct_last_us;
        }
        return true;
    }
    return false;
}
