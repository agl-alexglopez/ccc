#ifndef TEST
#define TEST

/* NOLINTBEGIN */
#include <stdio.h>
#include <stdlib.h>
/* NOLINTEND */

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

#define FORMAT(x)                                                              \
    _Generic((x),                                                              \
        _Bool: "%d",                                                           \
        char: "%c",                                                            \
        signed char: "%hhd",                                                   \
        unsigned char: "%hhu",                                                 \
        short: "%hd",                                                          \
        int: "%d",                                                             \
        long: "%ld",                                                           \
        long long: "%lld",                                                     \
        unsigned short: "%hu",                                                 \
        unsigned int: "%u",                                                    \
        unsigned long: "%lu",                                                  \
        unsigned long long: "%llu",                                            \
        float: "%f",                                                           \
        double: "%f",                                                          \
        long double: "%Lf",                                                    \
        char *: "%s",                                                          \
        char const *: "%s",                                                    \
        wchar_t *: "%ls",                                                      \
        wchar_t const *: "%ls",                                                \
        void *: "%p",                                                          \
        void const *: "%p",                                                    \
        default: "%p")

/* The CHECK macro evaluates a result against an expecation as one
   may be familiar with in many testing frameworks. However, it is
   expected to execute in a function where a test_result is returned.
   Provide the resulting operation against the expected outcome. The
   types must be comparable with ==/!=. Finally, provide the type and
   format specifier for the types being compared which also must be
   the same for both the result and expectation (e.g. int, "%d", size_t,
   "%zu"). If the result or expectation are function calls they will only
   be evaluated once. Finally, if memory has been allocted in the context of
   a test the requires a call to free, pass that free function and the memory
   pointers on which free must be called. Custom allocator functions are not
   supported and must override the library default free for now if such
   specialization is needed. */
#define CHECK(test_result, test_expected, optional_free_on_fail...)            \
    do                                                                         \
    {                                                                          \
        const __auto_type result_ = (test_result);                             \
        typeof(result_) const expected_ = (test_expected);                     \
        if (result_ != expected_)                                              \
        {                                                                      \
            char const *const format_string_ = FORMAT(result_);                \
            (void)fprintf(stderr, CYAN "--\nfailure in %s, line %d\n" NONE,    \
                          __func__, __LINE__);                                 \
            (void)fprintf(stderr,                                              \
                          GREEN "CHECK: "                                      \
                                "result( %s ) == expected( %s )" NONE "\n",    \
                          #test_result, #test_expected);                       \
            (void)fprintf(stderr, RED "ERROR: result( ");                      \
            (void)fprintf(stderr, format_string_, result_);                    \
            (void)fprintf(stderr, " ) != expected( ");                         \
            (void)fprintf(stderr, format_string_, expected_);                  \
            (void)fprintf(stderr, " )" CYAN "\n" NONE);                        \
            void *to_free_[] = {optional_free_on_fail};                        \
            unsigned const stop_ = sizeof(to_free_) / sizeof(void *);          \
            if (stop_)                                                         \
            {                                                                  \
                for (unsigned i_ = 0; i_ < stop_; i_++)                        \
                {                                                              \
                    free(to_free_[i_]);                                        \
                }                                                              \
            }                                                                  \
            return FAIL;                                                       \
        }                                                                      \
    } while (0)

#endif
