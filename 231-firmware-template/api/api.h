#include "stdio-text-protocol.h"

typedef void (*api_callback_t)(const command_t *command);

typedef struct
{
    const char *name;
    api_callback_t callback;
    const char *help;
} api_command_t;

void api_handle(const command_t *command);