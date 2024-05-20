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
set_cursor_position(const int row, const int col)
{
    printf("\033[%d;%df", row + 1, col + 1);
}

void
quit(const char *const msg)
{
    (void)fprintf(stdout, "%s", msg);
    exit(1);
}

struct int_conversion
convert_to_int(const char *const arg)
{
    char *end;
    const long conv = strtol(arg, &end, 10);
    if (errno == ERANGE)
    {
        (void)fprintf(stderr, "%s arg could not convert to int.\n", arg);
        return (struct int_conversion){.status = CONV_ER};
    }
    if (conv > INT_MAX)
    {
        (void)fprintf(stderr, "%s arg cannot exceed INT_MAX.\n", arg);
        return (struct int_conversion){.status = CONV_ER};
    }
    if (conv < INT_MIN)
    {
        (void)fprintf(stderr, "%s arg must exceed INT_MIN.\n", arg);
        return (struct int_conversion){.status = CONV_ER};
    }
    return (struct int_conversion){.status = CONV_OK, .conversion = (int)conv};
}
