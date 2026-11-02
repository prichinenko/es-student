#include "telemetry-task.h"
#include "platform/platform.h"
#include "systime/systime.h"

static uint32_t telemetry_period_ms;
static telemetry_state_t telemetry_state;
static uint64_t telemetry_next_us;
static telemetry_task_callback_t telemetry_callback;

void telemetry_task_init(telemetry_task_callback_t on_record)
{
    telemetry_period_ms = 10;
    telemetry_state = TELEMETRY_STATE_IDLE;
    telemetry_callback = on_record;
}

// вызывается на каждой итерации суперцикла, не ждёт
void telemetry_task_handle(void)
{
    uint64_t now = systime_us();
    if (telemetry_state == TELEMETRY_STATE_RUN && telemetry_next_us <= now)
    {
        telemetry_next_us += telemetry_period_ms * 1000;
        if (telemetry_callback)
        {
            telemetry_record_t record = {
                .time_us = now,
                .voltage_v = platform_voltage_read(),
                .temp_c = platform_temp_read()};
            telemetry_callback(&record);
        }
    }
}

// false — период не принят
bool telemetry_task_start(uint32_t period_ms)
{
    if (period_ms >= 10)
    {
        telemetry_period_ms = period_ms;
        telemetry_state = TELEMETRY_STATE_RUN;
        return true;
    }
    return false;
}

void telemetry_task_stop(void)
{
    telemetry_state = TELEMETRY_STATE_IDLE;
}

telemetry_state_t telemetry_task_get_state(void)
{
    return telemetry_state;
}

uint32_t telemetry_task_get_period_ms(void)
{
    return telemetry_period_ms;
}
