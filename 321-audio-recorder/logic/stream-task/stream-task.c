#include "stream-task.h"
#include "platform/platform.h"

static stream_state_t stream_state = STREAM_STATE_IDLE;
static uint32_t stream_samples;
static uint32_t stream_frames;
static uint32_t stream_bits;
static stream_task_callback_t stream_on_frame;

void stream_task_init(stream_task_callback_t on_frame)
{
    stream_samples = 0;
    stream_frames = 0;
    stream_bits = 16;
    stream_on_frame = on_frame;
}

// вызывается на каждой итерации суперцикла, не ждёт
void stream_task_handle(void)
{
    if (stream_state == STREAM_STATE_RUN)
    {
        uint32_t samples;
        const uint8_t *data = platform_stream_next(&samples);

        if (data)
        {
            stream_samples += samples;
            stream_header_t header = {
                .sync_word = STREAM_SYNC_WORD,
                .number = stream_frames,
                .count = samples};
            stream_on_frame(&header, data, samples * stream_bits / 8);
            stream_frames++;
            platform_stream_release();
        }
    }
}

// bits — 16 или 8; false — АЦП занят или разрядность не та
bool stream_task_start(uint8_t bits)
{
    if (bits != 8 && bits != 16)
    {
        return false;
    }
    if (platform_adc_busy())
    {
        return false;
    }
    if (!platform_stream_start(bits))
    {
        return false;
    }
    stream_samples = 0;
    stream_frames = 0;
    stream_bits = bits;
    stream_state = STREAM_STATE_RUN;
    return true;
}
void stream_task_stop(void)
{
    if (stream_state == STREAM_STATE_RUN)
    {
        platform_stream_stop();
    }
    stream_state = STREAM_STATE_IDLE;
}

stream_state_t stream_task_get_state(void)
{
    return stream_state;
}
uint8_t stream_task_get_bits(void)
{
    return stream_bits;
}

// итог потока с последнего запуска
uint32_t stream_task_get_frames(void)
{
    return stream_frames;
}
uint32_t stream_task_get_samples(void)
{
    return stream_samples;
}
uint32_t stream_task_get_overruns(void)
{
    return platform_stream_overruns();
}
