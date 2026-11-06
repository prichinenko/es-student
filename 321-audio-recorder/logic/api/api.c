#include "api.h"
#include "api-commands.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

void api_handle(const command_t *command)
{
    if (command->truncated)
    {
        printf("error: command longer than 63 characters or 4 arguments\n");
        return;
    }

    for (uint32_t i = 0; i < api_commands_count; i++)
    {
        if (strcmp(command->name, api_commands[i].name) == 0)
        {
            if (api_commands[i].callback != NULL)
            {
                api_commands[i].callback(command);
            }

            return;
        }
    }
    printf("error: unknown command '%s', try help\n", command->name);
}

void api_print_telemetry(const telemetry_record_t *record)
{
    printf("tm: %llu %.3f %.1f\n", record->time_us, record->voltage_v, record->temp_c);
}

void api_send_frame(const stream_header_t *header, const uint8_t *data, uint32_t bytes)
{
    platform_write(header, sizeof(*header));
    platform_write(data, bytes);
}