#ifndef TEST
#define TEST

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

/* The CHECK macro evaluates a result against an expecation as one
   may be familiar with in many testing frameworks. However, it is
   expected to execute in a function where a test_result is returned.
   Provide the resulting operation against the expected outcome. The
   types must be comparable with ==/!=. Finally, provide the type and
   format specifier for the types being compared which also must be
   the same for both RESULT and EXPECTED (e.g. int, "%d", size_t, "%zu",
   bool, "%b"). If RESULT or EXPECTED are function calls they will only
   be evaluated once, as expected. */
#define CHECK(user_test_result, user_test_expected, type_format_specifier)     \
    do                                                                         \
    {                                                                          \
        __auto_type const result_ = (user_test_result);                        \
        typeof(result_) const expected_ = (user_test_expected);                \
        if (result_ != expected_)                                              \
        {                                                                      \
            (void)fprintf(stderr, CYAN "--\nfailure in %s, line %d\n" NONE,    \
                          __func__, __LINE__);                                 \
            (void)fprintf(stderr,                                              \
                          GREEN "CHECK: "                                      \
                                "result( %s ) == expected( %s )" NONE "\n",    \
                          #user_test_result, #user_test_expected);             \
            (void)fprintf(stderr, RED "ERROR: result( ");                      \
            (void)fprintf(stderr, type_format_specifier, result_);             \
            (void)fprintf(stderr, " ) != expected( ");                         \
            (void)fprintf(stderr, type_format_specifier, expected_);           \
            (void)fprintf(stderr, " )" CYAN "\n" NONE);                        \
            return FAIL;                                                       \
        }                                                                      \
    } while (0)

#endif
