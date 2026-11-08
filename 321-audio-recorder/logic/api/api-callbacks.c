#include "api/api.h"
#include "device/device.h"
#include "systime/systime.h"
#include "led-task/led-task.h"
#include "button-task/button-task.h"
#include "pi-task/pi-task.h"
#include "profiling/profiling.h"
#include "platform/platform.h"
#include "telemetry-task/telemetry-task.h"
#include "block-task/block-task.h"
#include "stream-task/stream-task.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#define TELEMETRY_DEFAULT_PERIOD_MS 100
#define STREAM_BLINK_PERIOD_MS 250

// окно mic_level по умолчанию и окно mic_bias
#define MIC_DEFAULT_WINDOW_MS 100

static void error_adc_busy(void);
static bool start_block(uint32_t count, block_task_callback_t callback);

static void cmd_help(const command_t *command);
static void cmd_info(const command_t *command);
static void cmd_uptime(const command_t *command);
static void cmd_pi_start(const command_t *command);
static void cmd_pi(const command_t *command);
static void cmd_led_enable(const command_t *command);
static void cmd_led_disable(const command_t *command);
static void cmd_led_blink(const command_t *command);
static void cmd_led_period(const command_t *command);
static void cmd_button(const command_t *command);
static void cmd_profiling(const command_t *command);
static void cmd_profiling_reset(const command_t *command);
// напряжение на GP26 у platform, ответ voltage: <напряжение> V
static void cmd_get_voltage(const command_t *command);
// температура у platform, ответ temp: <температура> C
static void cmd_get_temp(const command_t *command);

static void cmd_telemetry_start(const command_t *command);
static void cmd_telemetry_stop(const command_t *command);

static void cmd_charge_on(const command_t *command);
static void cmd_charge_off(const command_t *command);

static void cmd_timer_blink_start(const command_t *command);
static void cmd_timer_blink_stop(const command_t *command);

static void cmd_cap_start(const command_t *command);
static void cmd_cap_dump(const command_t *command);
static void cmd_dma_cap(const command_t *command);

static void cmd_stream_start(const command_t *command);
static void cmd_stream_stop(const command_t *command);

static void cmd_generate_tone(const command_t *command);

static void cmd_hang(const command_t *command);
static void cmd_wdt_enable(const command_t *command);
static void cmd_reset_reason(const command_t *command);

static void cmd_mic_bias(const command_t *command);
static void cmd_mic_level(const command_t *command);

const api_command_t api_commands[] = {
    {"help", "list of commands", cmd_help},
    {"info", "device passport", cmd_info},
    {"uptime", "time since reset", cmd_uptime},
    {"led_enable", "led on", cmd_led_enable},
    {"led_disable", "led off", cmd_led_disable},
    {"led_blink", "led blink", cmd_led_blink},
    {"led_period", "led peruid ms: led_period [period]", cmd_led_period},
    {"button", "button status", cmd_button},
    {"pi_start", "start calculate pi: pi_start [terms]", cmd_pi_start},
    {"pi", "pi result", cmd_pi},
    {"profiling", "profiling", cmd_profiling},
    {"profiling_reset", "profiling reset", cmd_profiling_reset},
    {"get_voltage", "voltage on GP26, ADC input 0", cmd_get_voltage},
    {"get_temp", "chip temperature, degrees Celsius", cmd_get_temp},
    {"tm_start", "telemetry start [period]", cmd_telemetry_start},
    {"tm_stop", "telemetry stop [period]", cmd_telemetry_stop},
    {"charge_on", "charge on", cmd_charge_on},
    {"charge_off", "charge off", cmd_charge_off},
    {"timer_blink_start", "blink led by timer: timer_blink_start <period_ms>", cmd_timer_blink_start},
    {"timer_blink_stop", "stop timer blink, number of toggles", cmd_timer_blink_stop},
    {"cap_start", "capture start <N> <period_us>", cmd_cap_start},
    {"cap_dump", "capture start", cmd_cap_dump},
    {"dma_cap", "capture start", cmd_dma_cap},
    {"stream_start", "stream start [8|16]", cmd_stream_start},
    {"stream_stop", "stream start", cmd_stream_stop},
    {"generate_tone", "generate tone <Hz>", cmd_generate_tone},
    {"hang", "hang: forever loop", cmd_hang},
    {"wdt_enable", "hang: forever loop", cmd_wdt_enable},
    {"reset_reason", "hang: forever loop", cmd_reset_reason},
    {"mic_bias", "mic bias", cmd_mic_bias},
    {"mic_level", "nic lecel [<ms>]", cmd_mic_level},
};

const uint32_t api_commands_count = sizeof(api_commands) / sizeof(api_commands[0]);

// Число без знака из десятичной записи. false — в записи не только цифры
// или число не помещается в uint32_t.
static bool parse_u32(const char *text, uint32_t *value)
{
    // strtoul приняла бы и знак: из "-1" получилось бы 4294967295
    if (!isdigit((unsigned char)text[0]))
    {
        return false;
    }

    char *end;
    errno = 0;
    unsigned long number = strtoul(text, &end, 10);

    // после числа остались символы или число не поместилось
    if (*end != '\0' || errno == ERANGE || number > UINT32_MAX)
    {
        return false;
    }

    *value = (uint32_t)number;
    return true;
}

static void cmd_info(const command_t *command)
{

    device_info_t info;
    device_get_info(&info);

    printf("name: %s\n", info.name);
    printf("version: %s\n", info.version);
    printf("project: %s\n", info.project);
    printf("repo: %s\n", info.repo);
    printf("board: %s\n", info.board);
    printf("serial: %s\n", info.serial);
    printf(
        "chip: manufacturer 0x%03x, part 0x%04x, revision %u\n",
        (unsigned int)info.chip_manufacturer,
        (unsigned int)info.chip_part,
        (unsigned int)info.chip_revision);
    printf("pico-sdk: %s\n", info.pico_sdk);
}

static void cmd_uptime(const command_t *command)
{
    printf("uptime: %llu ms\n", (long long unsigned int)systime_ms());
}

static void cmd_pi_start(const command_t *command)
{
    uint32_t terms = 1000000;
    if (command->argc > 1)
    {
        printf("error: usage pi_start [terms]\n");
        return;
    }

    if (command->argc != 0)
    {
        if (!parse_u32(command->argv[0], &terms))
        {
            printf("error: usage pi_start [terms]\n");
            return;
        }
    }

    if (terms == 0)
    {
        printf("error: terms must be greater than 0\n");
        return;
    }

    if (pi_task_start(terms))
    {
        printf("pi: started, %u terms\n", (unsigned int)terms);
    }
    else
    {
        printf("error: pi is busy\n");
    }
}

static void cmd_pi(const command_t *command)
{
    pi_task_result_t result = pi_task_get_result();
    switch (result.state)
    {
    case PI_STATE_NOT_STARTED:
        printf("pi: not started\n");
        break;

    case PI_STATE_PROCESS:
        printf("pi: running\n");
        break;

    case PI_STATE_FINISH:
        printf("pi: %.9f in %llu ms\n", result.pi, result.time / 1000);
        break;
    }
}

static void cmd_help(const command_t *command)
{
    for (uint32_t i = 0; i < api_commands_count; i++)
    {
        printf("%-16s %s\n", api_commands[i].name, api_commands[i].help);
    }
}

static char *led_state_string(void)
{
    switch (led_task_get_state())
    {
    case LED_STATE_ON:
        return "on";
    case LED_STATE_OFF:
        return "off";
    case LED_STATE_BLINK:
        return "blink";
    }
    return "";
}

static void print_led_info(void)
{
    printf("led: %s, period %u ms\n", led_state_string(), (unsigned int)led_task_get_period_ms());
}

static void cmd_led_enable(const command_t *command)
{
    led_task_set_state(LED_STATE_ON);
    print_led_info();
}

static void cmd_led_disable(const command_t *command)
{
    led_task_set_state(LED_STATE_OFF);
    print_led_info();
}

static void cmd_led_blink(const command_t *command)
{
    led_task_set_state(LED_STATE_BLINK);
    print_led_info();
}

static void cmd_led_period(const command_t *command)
{
    uint32_t period = 0;
    if (command->argc != 1 || !parse_u32(command->argv[0], &period))
    {
        printf("error: usage led_period <period_ms>\n");
        return;
    }
    if (!period)
    {
        printf("error: period_ms must be greater than 0\n");
    }
    led_task_set_period_ms(period);
    print_led_info();
}

static void cmd_button(const command_t *command)
{
    printf(
        "button: %s, presses %u\n",
        button_task_is_pressed() ? "pressed" : "released",
        (unsigned int)button_task_get_press_count());
}

static void cmd_profiling(const command_t *command)
{
#if PROFILING_ENABLED
    for (uint32_t id = 0; id < STOPWATCH_COUNT; ++id)
    {
        profiling_result_t result;
        profiling_get(id, &result);
        printf(
            "%-8s count %u mean %.2f us max %u us\n",
            result.name,
            (unsigned int)result.n,
            result.avg_us,
            (unsigned int)result.max_us);
    }
#else
    printf("profiling is disabled in this build\n");
#endif
}

static void cmd_profiling_reset(const command_t *command)
{
    profiling_reset();
    printf("profiling reset\n");
}

static void cmd_get_voltage(const command_t *command)
{
    if (platform_adc_busy())
    {
        error_adc_busy();
        return;
    }

    printf("voltage: %.3f V\n", platform_voltage_read());
}

static void cmd_get_temp(const command_t *command)
{
    if (platform_adc_busy())
    {
        error_adc_busy();
        return;
    }

    printf("temp: %.1f C\n", platform_temp_read());
}

static void cmd_telemetry_start(const command_t *command)
{
    uint32_t period = 0;
    if (command->argc == 0)
    {
        period = TELEMETRY_DEFAULT_PERIOD_MS;
    }
    else if (command->argc > 1 || !parse_u32(command->argv[0], &period))
    {
        telemetry_task_stop();
        printf("error: usage tm_start [<period_ms>]\n");
        return;
    }
    if (period < 10)
    {
        telemetry_task_stop();
        printf("error: period_ms must be at least 10\n");
        return;
    }
    if (telemetry_task_start(period))
    {
        printf("telemetry: started, period %d ms\n", (int)telemetry_task_get_period_ms());
    }
    else
    {
        telemetry_task_stop();
        printf("error: period_ms must be at least 10\n");
    }
}

static void cmd_telemetry_stop(const command_t *command)
{
    telemetry_task_stop();
    printf("telemetry: stopped\n");
}

static void cmd_charge_on(const command_t *command)
{
    platform_charge_set(true);
    printf("charge: on\n");
}

static void cmd_charge_off(const command_t *command)
{
    platform_charge_set(false);
    printf("charge: off\n");
}

static void cmd_timer_blink_start(const command_t *command)
{
    uint32_t period = 0;
    const char *error1 = "error: period_ms must be 10..60000\n";
    const char *error2 = "error: usage timer_blink_start <period_ms>\n";

    if (command->argc == 0 || command->argc > 1 || !parse_u32(command->argv[0], &period))
    {
        (void)platform_blink_stop();
        printf(error2);
        return;
    }
    if (period < 10 || period > 60000)
    {
        (void)platform_blink_stop();
        printf(error1);
        return;
    }
    if (platform_blink_start(period))
    {
        printf("timer_blink: started, period %d ms\n", (int)platform_blink_period_ms());
    }
    else
    {
        (void)platform_blink_stop();
        printf(error2);
    }
}

static void cmd_timer_blink_stop(const command_t *command)
{
    printf("timer_blink: stopped, %d toggles\n", (int)platform_blink_stop());
}

static void cmd_cap_start(const command_t *command)
{
    uint32_t period = 0;
    uint32_t count = 0;
    const char *error1 = "error: N must be 1..2000, period_us at least 10\n";
    const char *error2 = "error: usage cap_start <N> <period_us>\n";

    if (platform_adc_busy())
    {
        error_adc_busy();
        return;
    }

    if (command->argc != 2 || !parse_u32(command->argv[0], &count) || !parse_u32(command->argv[1], &period))
    {
        printf(error2);
        return;
    }
    if (count < 1 || count > 2000)
    {
        printf(error1);
        return;
    }
    if (period < 10)
    {
        printf(error1);
        return;
    }
    if (platform_capture_start(count, period))
    {
        printf("capture: started, %d samples, period %d us\n", (int)count, (int)period);
    }
    else
    {
        printf(error2);
    }
}

static void cmd_cap_dump(const command_t *command)
{
    platform_capture_t capture;
    platform_capture_get(&capture);
    printf(
        "capture: %d samples, period %d us, interval %d..%d us, handler %.1f %%\n",
        (int)capture.count,
        (int)capture.period_us,
        (int)capture.min_interval_us,
        (int)capture.max_interval_us,
        capture.handler_percent);
    if (capture.count)
    {
        for (uint32_t n = 0; n <= capture.count >> 4; n++)
        {
            if ((n << 4) < capture.count)
            {
                printf("cap:");
                for (uint32_t i = (n << 4); i < ((n << 4) + 16) && i < capture.count; ++i)
                {
                    printf(" %d", (int)capture.samples[i]);
                }
                printf("\n");
            }
        }
    }
}

static void error_adc_busy(void)
{
    printf("error: adc busy\n");
}

void print_dma_cap(const uint16_t *samples, uint32_t count, uint32_t time_us)
{
    uint32_t min_val, max_val;
    float mean = 0.0f;

    min_val = max_val = samples[0];
    for (uint32_t i = 1; i < count; ++i)
    {
        if (min_val > samples[i])
        {
            min_val = samples[i];
        }
        if (max_val < samples[i])
        {
            max_val = samples[i];
        }
        mean += samples[i];
    }
    mean /= count;

    printf(
        "dma_cap: %d samples in %d us, expected %d us, min %d max %d mean %.1f\n",
        (int)count,
        (int)time_us,
        (int)((uint64_t)count * 1000000 / PLATFORM_SAMPLE_RATE_HZ),
        (int)min_val,
        (int)max_val,
        mean);
}

static void cmd_dma_cap(const command_t *command)
{
    uint32_t n;
    char error1[50];
    const char *error2 = "error: usage dma_cap <N>\n";
    snprintf(error1, sizeof(error1), "error: N must be 1..%d\n", PLATFORM_BLOCK_MAX);

    if (platform_adc_busy())
    {
        error_adc_busy();
        return;
    }

    if (command->argc != 1 || !parse_u32(command->argv[0], &n))
    {
        printf(error2);
        return;
    }
    if (n < 1 || n > PLATFORM_BLOCK_MAX)
    {
        printf(error1);
        return;
    }

    start_block(n, print_dma_cap);
}

static void cmd_stream_start(const command_t *command)
{
    uint32_t bits = 16;
    const char *error1 = "error: usage stream_start [8|16]\n";
    const char *error2 = "error: adc busy\n";

    if (command->argc == 1 && !parse_u32(command->argv[0], &bits))
    {
        printf(error1);
        return;
    }
    else if (command->argc > 1)
    {
        printf(error1);
        return;
    }
    if (bits != 8 && bits != 16)
    {
        printf(error1);
        return;
    }

    if (stream_task_start((uint8_t)bits))
    {
        platform_blink_start(STREAM_BLINK_PERIOD_MS);
        printf("stream: started, 44100 Hz, %d bit\n", (int)bits);
    }
    else
    {
        printf(error2);
    }
}

static void cmd_stream_stop(const command_t *command)
{
    stream_task_stop();
    platform_blink_stop();
    printf(
        "stream: stopped, %d frames, %d samples, %d overruns\n",
        (int)stream_task_get_frames(),
        (int)stream_task_get_samples(),
        (int)stream_task_get_overruns());
}

static void cmd_generate_tone(const command_t *command)
{
    uint32_t tone;
    const char *error1 = "error: usage generate_tone <Hz>\n";
    const char *error2 = "error: Hz must be 0 or 20..20000\n";

    if (command->argc != 1 || !parse_u32(command->argv[0], &tone))
    {
        printf(error1);
        return;
    }
    if (platform_tone_set(tone))
    {
        if (tone)
        {
            printf("tone: %d Hz\n", (int)tone);
        }
        else
        {
            printf("tone: off\n");
        }
    }
    else
    {
        printf(error2);
    }
}

static void cmd_hang(const command_t *command)
{
    printf("hang: superloop stopped\n");

    while (1)
    {
    }
}

static void cmd_wdt_enable(const command_t *command)
{
    uint32_t timeout_ms;
    const char *error1 = "error: usage wdt_enable <timeout_ms>\n";
    const char *error2 = "error: timeout_ms must be 1..8388\n";

    if (command->argc != 1 || !parse_u32(command->argv[0], &timeout_ms))
    {
        printf(error1);
        return;
    }
    if (platform_watchdog_enable(timeout_ms))
    {
        printf("wdt: enabled, timeout %d ms\n", (int)timeout_ms);
    }
    else
    {
        printf(error2);
    }
}

static void cmd_reset_reason(const command_t *command)
{
    uint32_t restarts;
    if (platform_watchdog_caused_reset(&restarts))
    {
        printf("reset: watchdog, restarts %d\n", (int)restarts);
    }
    else
    {
        printf("reset: other, restarts %d\n", (int)restarts);
    }
}

// среднее и отклонение от среднего в кодах АЦП: среднеквадратичное и наибольшее
static void block_stats(const uint16_t *samples, uint32_t count, float *mean, float *rms, float *peak)
{
    uint32_t sum = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        sum += samples[i];
    }
    int32_t center = (int32_t)(sum / count);

    // в целых числах: в float на 8192 отсчётах ядро считало бы в 24 раза дольше
    uint64_t squares = 0;
    uint32_t largest = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        int32_t deviation = (int32_t)samples[i] - center;
        uint32_t size = deviation < 0 ? -deviation : deviation;
        squares += (uint64_t)(deviation * deviation);
        if (size > largest)
        {
            largest = size;
        }
    }

    *mean = (float)sum / count;
    *rms = sqrtf((float)squares / count);
    *peak = (float)largest;
}

// среднее блока в вольтах, строка mic: bias <В, 3 знака> V
void print_mic_bias(const uint16_t *samples, uint32_t count, uint32_t time_us)
{
    float mean;
    float rms;
    float peak;

    block_stats(samples, count, &mean, &rms, &peak);

    printf("mic: bias %.3f V\n", platform_adc_volts(mean));
}

// отклонения в милливольтах, окно — count * 1000 / 44100,
//     строка mic: level <мВ, 1 знак> mV rms, peak <мВ, 1 знак> mV, <окно> ms
void print_mic_level(const uint16_t *samples, uint32_t count, uint32_t time_us)
{
    float mean;
    float rms;
    float peak;

    block_stats(samples, count, &mean, &rms, &peak);

    printf("mic: level %.1f mV rms, peak %.1f mV, %d ms\n",
           platform_adc_volts(rms) * 1000.0f,
           platform_adc_volts(peak) * 1000.0f,
           (int)((uint64_t)count * 1000 / PLATFORM_SAMPLE_RATE_HZ));
}

// захват 4410 отсчётов с функцией print_mic_bias
static void cmd_mic_bias(const command_t *command)
{
    start_block(MIC_DEFAULT_WINDOW_MS * PLATFORM_SAMPLE_RATE_HZ / 1000, print_mic_bias);
}

// больше одного аргумента или не число — error: usage mic_level [<ms>]
//     отсчётов окна 0 или больше 8192 — error: ms must be 1..185,
//     число отсчётов считать в uint64_t, аргумент может быть любым
//     захват с функцией print_mic_level
static void cmd_mic_level(const command_t *command)
{
    uint32_t ms = 100;
    const char *error1 = "error: usage mic_level [<ms>]\n";
    const char *error2 = "error: ms must be 1..185\n";

    if (command->argc != 0)
    {
        if (command->argc != 1 || !parse_u32(command->argv[0], &ms))
        {
            printf(error1);
            return;
        }
    }

    // uint64_t, чтобы ms * 44100 не переполнилось при любом аргументе
    uint64_t count = (uint64_t)ms * PLATFORM_SAMPLE_RATE_HZ / 1000;
    if (count == 0 || count > PLATFORM_BLOCK_MAX)
    {
        printf(error2);
        return;
    }

    start_block((uint32_t)count, print_mic_level);
}

static bool start_block(uint32_t count, block_task_callback_t callback)
{
    // телеметрия тоже читает АЦП, поэтому с ней захват не запускаем
    if (platform_adc_busy() || !block_task_start(count, callback))
    {
        error_adc_busy();
        return false;
    }
    return true;
}
