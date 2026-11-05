#include "block-task.h"
#include "platform/platform.h"

typedef enum
{
    BLOCK_TASK_STATE_IDLE,
    BLOCK_TASK_STATE_RUN,
} block_task_state_t;

static block_task_state_t block_task_state = BLOCK_TASK_STATE_IDLE;
static block_task_callback_t block_task_callback;
static uint32_t block_task_count;

// готовый блок: отсчёты, их число и время захвата
typedef void (*block_task_callback_t)(const uint16_t *samples, uint32_t count, uint32_t time_us);

// false — АЦП занят или блок больше, чем помещается в буфер
bool block_task_start(uint32_t count, block_task_callback_t on_block)
{
    if (platform_adc_busy() || count > PLATFORM_BLOCK_MAX)
    {
        block_task_state = BLOCK_TASK_STATE_IDLE;
        return false;
    }
    if (platform_block_start(count))
    {
        block_task_count = count;
        block_task_callback = on_block;
        block_task_state = BLOCK_TASK_STATE_RUN;
        return true;
    }
    block_task_state = BLOCK_TASK_STATE_IDLE;
    return false;
}

// вызывается на каждой итерации суперцикла, не ждёт
void block_task_handle(void)
{
    if (block_task_state == BLOCK_TASK_STATE_RUN && platform_block_done())
    {
        block_task_state = BLOCK_TASK_STATE_IDLE;
        block_task_callback(
            platform_block_samples(),
            block_task_count,
            platform_block_time_us());
    }
}