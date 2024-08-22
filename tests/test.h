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

/* ccc_set this breakpoint on any line where you wish
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
#define CHECK(result, expected, type_format_specifier)                         \
    do                                                                         \
    {                                                                          \
        typeof(result) const _result_ = result;                                \
        typeof(result) const _expected_ = expected;                            \
        if (_result_ != _expected_)                                            \
        {                                                                      \
            (void)fprintf(stderr, CYAN "--\nfailure in %s, line %d\n" NONE,    \
                          __func__, __LINE__);                                 \
            (void)fprintf(stderr,                                              \
                          GREEN "CHECK: "                                      \
                                "result( %s ) == expected( %s )" NONE "\n",    \
                          #result, #expected);                                 \
            (void)fprintf(stderr, RED "ERROR: result( ");                      \
            (void)fprintf(stderr, type_format_specifier, _result_);            \
            (void)fprintf(stderr, " ) != expected( ");                         \
            (void)fprintf(stderr, type_format_specifier, _expected_);          \
            (void)fprintf(stderr, " )" CYAN "\n" NONE);                        \
            return FAIL;                                                       \
        }                                                                      \
    } while (0)

#endif
