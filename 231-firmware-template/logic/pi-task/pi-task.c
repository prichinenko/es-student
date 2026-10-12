#include "pi-task.h"
#include "systime/systime.h"
#include <stdio.h>

/*
 * Ценеа члена ряда 585 тактов. При 125МГц это 4,7мкс.
 * Обработчик задачи долэен уложиться в 0,5мс, это около 100 членов.
 */
static const uint32_t PI_TERMS_PER_CALL = 100;

static pi_state_t pi_state;
static uint64_t pi_time_start;
static uint64_t pi_time_finish;
static uint32_t pi_terms;
static uint32_t pi_k;
static double pi;
static double pi_process_sum = 0.0;
static double pi_process_sign = 1.0;

static void pi_task_process(void)
{
    if (pi_state == PI_STATE_PROCESS)
    {
        uint32_t pi_terms_step_end = pi_k + PI_TERMS_PER_CALL;
        if (pi_terms_step_end > pi_terms)
        {
            pi_terms_step_end = pi_terms;
        }

        while (pi_k < pi_terms_step_end)
        {
            // Прибавить очередной член ряда: ±1 / (2k + 1)
            pi_process_sum += (double)pi_process_sign / ((double)(pi_k << 1) + 1.0);
            pi_process_sign = -pi_process_sign;
            pi_k++;

            // printf("%lu: %f\n", pi_k, pi_process_sum);
        }

        if (pi_k >= pi_terms)
        {
            pi_process_sum += 0.5 * (double)pi_process_sign / ((double)(pi_k << 1) + 1.0);
            pi = 4.0 * pi_process_sum;
            pi_time_finish = systime_us();
            pi_state = PI_STATE_FINISH;
        }
    }
}

void pi_task_init(void)
{
    pi_state = PI_STATE_NOT_STARTED;
    pi = 0.0f;
    pi_time_start = 0;
    pi_time_finish = 0;
}

void pi_task_handle(void)
{
    if (pi_state == PI_STATE_PROCESS)
    {
        pi_task_process();
    }
}

bool pi_task_start(uint32_t terms)
{
    if (pi_state == PI_STATE_PROCESS)
    {
        return false;
    }
    pi_state = PI_STATE_PROCESS;
    pi_k = 0;
    pi_terms = terms;
    pi = 0.0;
    pi_process_sum = 0.0;
    pi_process_sign = 1.0;
    pi_time_start = systime_us();
    return true;
}

const pi_task_result_t pi_task_get_result(void)
{
    pi_task_result_t pi_result;
    pi_result.pi = pi;
    pi_result.time = pi_time_finish - pi_time_start;
    pi_result.state = pi_state;
    return pi_result;
}