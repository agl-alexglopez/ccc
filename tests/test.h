#ifndef TEST
#define TEST

#include <signal.h>
#include <stdio.h>

#define RED "\033[38;5;9m"
#define GREEN "\033[38;5;10m"
#define CYAN "\033[38;5;14m"
#define NONE "\033[0m"

enum test_result
{
    ERROR = -1,
    PASS,
    FAIL,
};

typedef enum test_result (*test_fn)(void);

struct fn_name
{
    test_fn fn;
    const char *const name;
};

/* Set this breakpoint on any line where you wish
   execution to stop. Under normal program runs the program
   will simply exit. If triggered in GDB execution will stop
   while able to explore the surrounding context, varialbes,
   and stack frames. Be sure to step "(gdb) up" out of the
   raise function to wherever it triggered. */
#define BREAKPOINT()                                                           \
    do                                                                         \
    {                                                                          \
        (void)fprintf(stderr, "\n!!Break. Line: %d File: %s, Func: %s\n ",     \
                      __LINE__, __FILE__, __func__);                           \
        (void)raise(SIGTRAP);                                                  \
    } while (0)

/* The CHECK macro evaluates a result against an expecation as one
   may be familiar with in many testing frameworks. However, it is
   expected to execute in a function where a test_result is returned.
   Provide the resulting operation against the expected outcome. The
   types must be comparable with ==/!=. Finally, provide the type and
   format specifier for the types being compared which also must be
   the same for both RESULT and EXPECTED (e.g. int, "%d", size_t, "%zu",
   bool, "%b"). If RESULT or EXPECTED are function calls they will only
   be evaluated once, as expected. */
#define CHECK(RESULT, EXPECTED, TYPE, TYPE_FORMAT_SPECIFIER)                   \
    do                                                                         \
    {                                                                          \
        const TYPE r_unique_name_macro = RESULT;                               \
        const TYPE e_unique_name_macro = EXPECTED;                             \
        if (r_unique_name_macro != e_unique_name_macro)                        \
        {                                                                      \
            (void)fprintf(stderr,                                              \
                          CYAN "--\n" GREEN "CHECK: "                          \
                               "RESULT( %s ) == EXPECTED( %s )" NONE "\n",     \
                          #RESULT, #EXPECTED);                                 \
            (void)fprintf(stderr, RED "ERROR: RESULT( ");                      \
            (void)fprintf(stderr, TYPE_FORMAT_SPECIFIER, r_unique_name_macro); \
            (void)fprintf(stderr, " ) != EXPECTED( ");                         \
            (void)fprintf(stderr, TYPE_FORMAT_SPECIFIER, e_unique_name_macro); \
            (void)fprintf(stderr, " )" CYAN "\n" NONE);                        \
            (void)fprintf(stderr, CYAN "see line %d\n" NONE, __LINE__);        \
            return FAIL;                                                       \
        }                                                                      \
    } while (0)

#endif
