#include "platform.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/timer.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "hardware/watchdog.h"

#define LED_PIN 25
#define BUTTON_PIN 10

#define CHARGE_PIN 22

// вход АЦП: вывод GP26, канал 0
#define ADC_PIN 26
#define ADC_CHANNEL 0

// опорное напряжение АЦП на плате Pico и число уровней 12-разрядного кода
#define ADC_VREF_V 3.3f
#define ADC_LEVELS 4096

// встроенный датчик температуры — канал 4
#define ADC_TEMP_CHANNEL 4

#define TONE_PIN 20
#define TONE_COUNT_HZ 1000000

static repeating_timer_t blink_timer;
static bool blink_running;
static volatile bool blink_on;
static volatile uint32_t blink_toggles;

// кто сейчас работает с АЦП; меняют и функции, и обработчики прерываний
typedef enum
{
    ADC_FREE,
    ADC_CAPTURE,
    ADC_BLOCK,
    ADC_STREAM,
} adc_user_t;

static volatile adc_user_t adc_user;

static repeating_timer_t capture_timer;
static uint16_t capture_samples[PLATFORM_CAPTURE_MAX];
static uint32_t capture_times[PLATFORM_CAPTURE_MAX];
static uint32_t capture_count;
static uint32_t capture_period_us;
static volatile uint32_t capture_index;
static volatile uint32_t capture_handler_us;

// частота clk_adc, от неё делитель отсчитывает шаг
#define ADC_CLOCK_HZ 48000000.0f

static int block_channel;
static uint16_t block_samples[PLATFORM_BLOCK_MAX];
static volatile uint32_t block_start_us;
static volatile uint32_t block_end_us;
static volatile bool block_ready;

static int stream_channels[2];
static uint8_t stream_buffers[2][PLATFORM_STREAM_BYTES] __attribute__((aligned(4)));
static volatile bool stream_ready[2];
static volatile uint32_t stream_overruns;
static uint32_t stream_samples;
static uint32_t stream_next;

static void on_block_done(void);
static void on_stream_buffer(void);

#define WATCHDOG_MAX_MS 8388

static bool watchdog_reset;
static uint32_t watchdog_restarts;

void platform_init(void)
{
    // причина прошлого сброса; счётчик сбросов подряд хранит scratch-регистр 0,
    // его не стирает сброс по сторожевому таймеру
    watchdog_reset = watchdog_enable_caused_reboot();
    if (watchdog_reset)
    {
        watchdog_hw->scratch[0]++;
    }
    else
    {
        watchdog_hw->scratch[0] = 0;
    }
    watchdog_restarts = watchdog_hw->scratch[0];

    stdio_init_all();

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    platform_led_set(true);

    gpio_init(CHARGE_PIN);
    gpio_set_dir(CHARGE_PIN, GPIO_OUT);
    platform_charge_set(false);

    adc_init();
    adc_gpio_init(ADC_PIN);

    adc_set_temp_sensor_enabled(true);

    // канал блока сообщает о конце через DMA_IRQ_1
    block_channel = dma_claim_unused_channel(true);
    dma_channel_set_irq1_enabled(block_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_1, on_block_done);
    irq_set_enabled(DMA_IRQ_1, true);

    // два канала потока сообщают о заполненном буфере через DMA_IRQ_0
    stream_channels[0] = dma_claim_unused_channel(true);
    stream_channels[1] = dma_claim_unused_channel(true);
    irq_set_exclusive_handler(DMA_IRQ_0, on_stream_buffer);
    irq_set_enabled(DMA_IRQ_0, true);

    gpio_set_function(TONE_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(TONE_PIN);
    pwm_set_clkdiv(slice, (float)clock_get_hz(clk_sys) / TONE_COUNT_HZ);
    pwm_set_wrap(slice, 999);
    pwm_set_gpio_level(TONE_PIN, 0);
    pwm_set_enabled(slice, true);
}

void platform_led_set(bool on)
{
    gpio_put(LED_PIN, on);
}

bool platform_led_get(void)
{
    return !gpio_get(LED_PIN);
}

bool platform_button_read(void)
{
    return !gpio_get(BUTTON_PIN);
}

float platform_voltage_read(void)
{
    adc_select_input(ADC_CHANNEL);
    return adc_read() * ADC_VREF_V / ADC_LEVELS;
}

// температура кристалла по встроенному датчику, градусы Цельсия
float platform_temp_read(void)
{
    adc_select_input(ADC_TEMP_CHANNEL);
    return 27 - (adc_read() * ADC_VREF_V / ADC_LEVELS - 0.706) / 0.001721;
}

void platform_charge_set(bool on)
{
    gpio_put(CHARGE_PIN, on);
}

// обработчик будильника индикатора
static bool on_blink(repeating_timer_t *timer)
{
    blink_on = !blink_on;
    gpio_put(LED_PIN, blink_on);
    blink_toggles++;

    // true — вызвать снова через тот же период
    return true;
}

uint32_t platform_blink_period_ms(void)
{
    return -blink_timer.delay_us / 1000;
}

bool platform_blink_start(uint32_t period_ms)
{
    if (period_ms < 10 || period_ms > 60000)
    {
        return false;
    }

    platform_blink_stop();
    blink_toggles = 0;

    // минус перед периодом — отсчитывать от начала прошлого вызова
    blink_running = add_repeating_timer_ms(-(int32_t)period_ms, on_blink, NULL, &blink_timer);
    return blink_running;
}

uint32_t platform_blink_stop(void)
{
    if (blink_running)
    {
        cancel_repeating_timer(&blink_timer);
        blink_running = false;
    }

    blink_on = false;
    gpio_put(LED_PIN, 0);
    return blink_toggles;
}

// обработчик будильника захвата: время и отсчёт, затем время своей работы
static bool on_capture(repeating_timer_t *timer)
{
    uint32_t start_us = time_us_32();
    capture_times[capture_index] = start_us;
    capture_samples[capture_index] = adc_read();
    capture_index++;
    capture_handler_us += time_us_32() - start_us;

    if (capture_index < capture_count)
    {
        return true;
    }

    // захват окончен: АЦП свободен, будильник снимается
    adc_user = ADC_FREE;
    return false;
}

bool platform_capture_start(uint32_t count, uint32_t period_us)
{
    if (adc_user != ADC_FREE || count == 0 || count > PLATFORM_CAPTURE_MAX || period_us == 0)
    {
        return false;
    }

    adc_select_input(ADC_CHANNEL);
    capture_count = count;
    capture_period_us = period_us;
    capture_index = 0;
    capture_handler_us = 0;
    adc_user = ADC_CAPTURE;

    if (!add_repeating_timer_us(-(int64_t)period_us, on_capture, NULL, &capture_timer))
    {
        adc_user = ADC_FREE;
        return false;
    }
    return true;
}

bool platform_adc_busy(void)
{
    return adc_user != ADC_FREE;
}

bool platform_capture_done(void)
{
    return capture_index == capture_count;
}

void platform_capture_get(platform_capture_t *capture)
{
    uint32_t min_interval = 0;
    uint32_t max_interval = 0;
    float handler_percent = 0.0f;

    capture->samples = capture_samples;
    capture->period_us = capture_period_us;
    capture->count = capture_index;

    if (capture->count > 1)
    {
        min_interval = max_interval = capture_times[1] - capture_times[0];
        for (uint32_t i = 2; i < capture->count; ++i)
        {
            uint32_t delta = capture_times[i] - capture_times[i - 1];
            if (delta > max_interval)
            {
                max_interval = delta;
            }
            if (delta < min_interval)
            {
                min_interval = delta;
            }
        }

        handler_percent = 100 * capture_handler_us / (capture_times[capture_index - 1] - capture_times[0]);
    }

    capture->max_interval_us = max_interval;
    capture->min_interval_us = min_interval;
    capture->handler_percent = handler_percent;
}

// АЦП в непрерывном режиме с шагом 44,1 кГц, буфер АЦП с запросом DMA
// на каждый отсчёт
static void adc_free_running(void)
{
    adc_select_input(ADC_CHANNEL);
    adc_fifo_setup(true, true, 1, false, false);
    adc_set_clkdiv(ADC_CLOCK_HZ / PLATFORM_SAMPLE_RATE_HZ - 1);
}

// АЦП обратно в режим отсчётов по команде
static void adc_one_shot(void)
{
    adc_run(false);
    adc_fifo_drain();
    adc_fifo_setup(false, false, 0, false, false);
    adc_set_clkdiv(0);
}

// обработчик прерывания DMA_IRQ_1: блок захвачен
static void on_block_done(void)
{
    dma_channel_acknowledge_irq1(block_channel);
    block_end_us = time_us_32();
    adc_one_shot();
    block_ready = true;
    adc_user = ADC_FREE;
}

bool platform_block_start(uint32_t count)
{
    if (adc_user != ADC_FREE || count == 0 || count > PLATFORM_BLOCK_MAX)
    {
        return false;
    }

    adc_free_running();

    // 16-битные передачи из одного адреса в растущий, по запросу АЦП
    dma_channel_config cfg = dma_channel_get_default_config(block_channel);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_dreq(&cfg, DREQ_ADC);

    block_ready = false;
    adc_user = ADC_BLOCK;

    // куда, откуда, сколько; true — запустить сразу
    dma_channel_configure(block_channel, &cfg, block_samples, &adc_hw->fifo, count, true);
    block_start_us = time_us_32();
    adc_run(true);
    return true;
}

bool platform_block_done(void)
{
    return block_ready;
}

const uint16_t *platform_block_samples(void)
{
    return block_samples;
}

// время захвата блока, от запуска АЦП до последнего отсчёта
uint32_t platform_block_time_us(void)
{
    return block_end_us - block_start_us;
}

// обработчик прерывания DMA_IRQ_0: канал заполнил свой буфер
static void on_stream_buffer(void)
{
    for (int i = 0; i < 2; i++)
    {
        if (dma_channel_get_irq0_status(stream_channels[i]))
        {
            dma_channel_acknowledge_irq0(stream_channels[i]);

            // адрес записи дошёл до конца буфера, вернуть его к началу
            dma_channel_set_write_addr(stream_channels[i], stream_buffers[i], false);

            // в следующий буфер уже идёт запись, а его ещё не отправили
            if (stream_ready[(i + 1) % 2])
            {
                stream_overruns++;
            }
            stream_ready[i] = true;
        }
    }
}

bool platform_stream_start(uint8_t bits)
{
    if (adc_user != ADC_FREE || bits != 16)
    {
        return false;
    }

    stream_samples = PLATFORM_STREAM_BYTES / 2;
    adc_free_running();

    // канал i пишет в буфер i и по окончании запускает другой канал
    for (int i = 0; i < 2; i++)
    {
        // номер следующего канала в цепочке
        int next_channel = stream_channels[(i + 1) % 2];

        dma_channel_config cfg = dma_channel_get_default_config(stream_channels[i]);
        channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
        channel_config_set_read_increment(&cfg, false);
        channel_config_set_write_increment(&cfg, true);
        channel_config_set_dreq(&cfg, DREQ_ADC);
        channel_config_set_chain_to(&cfg, next_channel);
        dma_channel_configure(stream_channels[i], &cfg, stream_buffers[i], &adc_hw->fifo,
                              stream_samples, false);
        dma_channel_set_irq0_enabled(stream_channels[i], true);
        stream_ready[i] = false;
    }

    stream_overruns = 0;
    stream_next = 0;
    adc_user = ADC_STREAM;
    dma_channel_start(stream_channels[0]);
    adc_run(true);
    return true;
}

void platform_stream_stop(void)
{
    if (adc_user != ADC_STREAM)
    {
        return;
    }

    // сначала остановить отсчёты, затем каналы и их прерывания
    adc_run(false);

    // RP2040-E13: передача, начатая до прерывания канала, завершается как обычная
    // и запускает канал из цепочки. Поэтому цепочка сначала разрывается: каждый
    // канал указывает сам на себя, иначе один из них останется взведённым
    for (int i = 0; i < 2; i++)
    {
        dma_channel_config cfg = dma_get_channel_config(stream_channels[i]);
        channel_config_set_chain_to(&cfg, stream_channels[i]);
        dma_channel_set_config(stream_channels[i], &cfg, false);
    }

    for (int i = 0; i < 2; i++)
    {
        dma_channel_set_irq0_enabled(stream_channels[i], false);
        dma_channel_abort(stream_channels[i]);
        dma_channel_acknowledge_irq0(stream_channels[i]);
    }
    adc_one_shot();
    adc_user = ADC_FREE;
}

void platform_write(const void *data, uint32_t bytes)
{
    // false, false — без перевода строки в конце и без замены \n на \r\n
    stdio_put_string((const char *)data, (int)bytes, false, false);
}

// следующий заполненный буфер по порядку или NULL, если он ещё не готов;
// samples — число отсчётов в буфере
const uint8_t *platform_stream_next(uint32_t *samples)
{
    if (stream_ready[stream_next])
    {
        *samples = stream_samples;
        return stream_buffers[stream_next];
    }
    return NULL;
}

// буфер отправлен, его можно заполнять снова
void platform_stream_release(void)
{
    stream_ready[stream_next] = false;
    stream_next ^= 1;
}

// сколько раз DMA застал буфер неотправленным
uint32_t platform_stream_overruns(void)
{
    return stream_overruns;
}

bool platform_tone_set(uint32_t tone)
{
    if (tone != 0 && (tone < PLATFORM_TONE_MIN_HZ || tone > PLATFORM_TONE_MAX_HZ))
    {
        return false;
    }

    if (tone == 0)
    {
        pwm_set_gpio_level(TONE_PIN, 0);
    }
    else
    {
        uint32_t period = (TONE_COUNT_HZ + (tone >> 1)) / tone;
        uint slice = pwm_gpio_to_slice_num(TONE_PIN);
        pwm_set_wrap(slice, period - 1);
        pwm_set_gpio_level(TONE_PIN, period / 2);
        pwm_set_enabled(slice, true);
    }
    return true;
}

// сторожевой таймер; false — таймаут больше 8388 мс или равен нулю
bool platform_watchdog_enable(uint32_t timeout_ms)
{
    if (timeout_ms == 0 || timeout_ms > WATCHDOG_MAX_MS)
    {
        return false;
    }

    // true — не считать, пока ядро остановлено отладчиком
    watchdog_enable(timeout_ms, true);
    return true;
}

void platform_watchdog_feed(void)
{
    watchdog_update();
}

// был ли последний сброс по сторожевому таймеру и сколько таких сбросов подряд
bool platform_watchdog_caused_reset(uint32_t *restarts)
{
    *restarts = watchdog_restarts;
    return watchdog_reset;
}
