#pragma once

#include "stdio-text-protocol/stdio-text-protocol.h"
#include "telemetry-task/telemetry-task.h"
#include "stream-task/stream-task.h"

typedef void (*api_callback_t)(const command_t *command);

typedef struct
{
    const char *name;
    const char *help;
    api_callback_t callback;
} api_command_t;

void api_handle(const command_t *command);

// печатает запись телеметрии строкой tm: <время, мкс> <напряжение, В> <температура, °C>
void api_print_telemetry(const telemetry_record_t *record);

void api_send_frame(const stream_header_t *header, const uint8_t *data, uint32_t bytes);