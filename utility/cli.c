#include "cli.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

void
clear_screen(void)
{
    printf("\033[2J\033[1;1H");
}

void
clear_line(void)
{
    printf("\033[2K\r");
}

void
set_cursor_position(int const row, int const col)
{
    printf("\033[%d;%df", row + 1, col + 1);
}

void
quit(char const *const message, int code)
{
    (void)fprintf(stdout, "%s", message);
    exit(code);
}

struct Int_conversion
convert_to_int(char const *const arg)
{
    char *end;
    long const conv = strtol(arg, &end, 10);
    if (errno == ERANGE)
    {
        (void)fprintf(stderr, "%s arg could not convert to int.\n", arg);
        return (struct Int_conversion){.status = CONV_ER};
    }
    if (conv > INT_MAX)
    {
        (void)fprintf(stderr, "%s arg cannot exceed INT_MAX.\n", arg);
        return (struct Int_conversion){.status = CONV_ER};
    }
    if (conv < INT_MIN)
    {
        (void)fprintf(stderr, "%s arg must exceed INT_MIN.\n", arg);
        return (struct Int_conversion){.status = CONV_ER};
    }
    return (struct Int_conversion){.status = CONV_OK, .conversion = (int)conv};
}
