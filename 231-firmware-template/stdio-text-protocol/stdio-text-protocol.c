#include "stdio-text-protocol.h"
#include <stdio.h>
#include <stdint.h>
#include "pico/stdio.h"
// #include "pico/stdlib.h"
// #include <string.h>

#define PICO_ERROR_TIMEOUT -2

char line[STDIO_TEXT_PROTOCOL_LINE_SIZE];
uint8_t line_length = 0;
bool line_overflow = false;

void stdio_text_protocol_init(void)
{
}

static const command_t *parse_command(void)
{
    static command_t cmd;
    cmd.name = NULL;
    cmd.argc = 0;
    cmd.truncated = line_overflow;

    for (uint8_t i = 0; i < STDIO_TEXT_PROTOCOL_MAX_ARGS; ++i)
    {
        cmd.argv[i] = NULL;
    }

    char *p = line;
    while (*p == ' ')
        p++;
    cmd.name = p;

    while (*p && *p != ' ')
        p++;
    if (*p == ' ')
    {
        *p = '\0';
        p++;
    }

    while (*p)
    {
        while (*p == ' ')
            p++;
        if (!*p)
            break;

        if (cmd.argc < STDIO_TEXT_PROTOCOL_MAX_ARGS)
        {
            cmd.argv[cmd.argc++] = p;
        }
        else
        {
            cmd.truncated = true;
        }

        while (*p && *p != ' ')
            p++;
        if (*p == ' ')
        {
            *p = '\0';
            p++;
        }
    }

    return &cmd;
}

const command_t *stdio_text_protocol_handle(void)
{
    // читаем символ без ожидания; нет символа — возвращаем NULL

    // конец строки: пустую строку пропускаем, иначе переводим строку в терминале,
    // делим строку на слова и возвращаем команду

    // Backspace (0x08 или 0x7f): убираем последний символ и стираем его на экране "\b \b"

    // печатный символ: кладём в буфер и повторяем в терминал;
    // буфер полон — помечаем строку как переполненную

    int symbol = getchar_timeout_us(0);

    if (symbol == PICO_ERROR_TIMEOUT)
    {
        return NULL;
    }

    // Backspace (0x08 или 0x7f): убираем последний символ и стираем его на экране
    if (symbol == 0x08 || symbol == 0x7f)
    {
        if (line_length > 0)
        {
            line_length--;
            // "\b \b" — возвращаем курсор, затираем пробелом, возвращаем обратно
            putchar('\b');
            putchar(' ');
            putchar('\b');
            if (line_length < STDIO_TEXT_PROTOCOL_LINE_SIZE - 1)
            {
                line_overflow = false;
            }
        }
        return NULL;
    }

    // Конец строки
    if (symbol == '\r' || symbol == '\n')
    {
        line[line_length] = '\0';

        if (line_length > 0)
        {
            // putchar('\n');
            const command_t *cmd = parse_command();
            line_length = 0;
            line_overflow = false;
            return cmd;
        }

        line_length = 0;
        line_overflow = false;
        return NULL;
    }

    // Печатный символ: кладём в буфер и повторяем в терминал;
    // буфер полон — помечаем строку как переполненную
    if (line_length < STDIO_TEXT_PROTOCOL_LINE_SIZE - 1)
    {
        line[line_length++] = (char)symbol;
        // putchar(symbol);
        line_overflow = false;
    }
    else
    {
        line_overflow = true;
    }

    return NULL;
}