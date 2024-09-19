#ifndef CCC_TEST
#define CCC_TEST

/* NOLINTBEGIN */
#include <stdio.h>
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

#define TEST_PRINT_FAIL(result, result_string, expected, expected_string)      \
    do                                                                         \
    {                                                                          \
        char const *const format_string_ = _Generic((result_),                 \
            _Bool: "%d",                                                       \
            char: "%c",                                                        \
            signed char: "%hhd",                                               \
            unsigned char: "%hhu",                                             \
            short: "%hd",                                                      \
            int: "%d",                                                         \
            long: "%ld",                                                       \
            long long: "%lld",                                                 \
            unsigned short: "%hu",                                             \
            unsigned int: "%u",                                                \
            unsigned long: "%lu",                                              \
            unsigned long long: "%llu",                                        \
            float: "%f",                                                       \
            double: "%f",                                                      \
            long double: "%Lf",                                                \
            char *: "%s",                                                      \
            char const *: "%s",                                                \
            wchar_t *: "%ls",                                                  \
            wchar_t const *: "%ls",                                            \
            void *: "%p",                                                      \
            void const *: "%p",                                                \
            default: "%p");                                                    \
        (void)fprintf(stderr, CYAN "--\nfailure in %s, line %d\n" NONE,        \
                      __func__, __LINE__);                                     \
        (void)fprintf(stderr,                                                  \
                      GREEN "CHECK: "                                          \
                            "result( %s ) == expected( %s )" NONE "\n",        \
                      result_string, expected_string);                         \
        (void)fprintf(stderr, RED "ERROR: result( ");                          \
        (void)fprintf(stderr, format_string_, result_);                        \
        (void)fprintf(stderr, " ) != expected( ");                             \
        (void)fprintf(stderr, format_string_, expected_);                      \
        (void)fprintf(stderr, " )" CYAN "\n" NONE);                            \
    } while (0)

#define NON_DEFAULT_PARAMS(...) __VA_ARGS__
#define DEFAULT_PARAMS(...) void
#define OPTIONAL_PARAMS(...) __VA_OPT__(NON_)##DEFAULT_PARAMS(__VA_ARGS__)

/** @brief Define a static test function that has a name and optionally
additional parameters that one may wish to define for a test function.
@param [in] test_name the name of the static function.
@param [in] ... any additional parameters required for the function.
@return see the end test macro. This will return a enum test_result that is
PASS or FAIL to be handled as the user sees fit.

It is possible to return early from a test before the end test macro, but it
is discouraged, especially if any memory allocations need to be cleaned up if
a test fails.

Example with no additional parameters:

BEGIN_STATIC_TEST(fhash_test_empty)
{
    struct val vals[5] = {};
    ccc_flat_hash_map fh;
    ccc_result const res
        = fhm_init(&fh, vals, sizeof(vals) / sizeof(vals[0]), struct val, id, e,
                   NULL, fhash_int_zero, fhash_id_eq, NULL);
    CHECK(empty(&fh), true);
    END_TEST();
}

Example with multiple parameters:

enum test_result insert_shuffled(ccc_double_ended_priority_queue *,
                                 struct val[], size_t, int);

BEGIN_STATIC_TEST(insert_shuffled, ccc_double_ended_priority_queue *pq,
                  struct val vals[], size_t const size, int const larger_prime)
{
    for (int i = 0 shuffled_index = larger_prime % size; i < size; ++i)
    {
        vals[shuffled_index].val = shuffled_index;
        push(pq, &vals[shuffled_index].elem);
        CHECK(ccc_depq_validate(pq), true);
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    END_TEST();
}*/
#define BEGIN_STATIC_TEST(test_name, ...)                                      \
    static enum test_result(test_name)(OPTIONAL_PARAMS(__VA_ARGS__))           \
    {                                                                          \
        enum test_result macro_test_res_ = PASS;                               \
        do

/** @brief Define a test function that has a name and optionally
additional parameters that one may wish to define a test function.
@param [in] test_name the name of the function.
@param [in] ... any additional parameters required for the function.
@return see the end test macro. This will return a enum test_result.

It is possible to return early from a test before the end test macro, but it
is discouraged, especially if any memory allocations need to be cleaned up if
a test fails. See begin static test for examples. */
#define BEGIN_TEST(test_name, ...)                                             \
    enum test_result(test_name)(OPTIONAL_PARAMS(__VA_ARGS__))                  \
    {                                                                          \
        enum test_result macro_test_res_ = PASS;                               \
        do

/** @brief execute a check within the context of a test.
@param [in] test_result the result value of some action.
@param [in] test_expected the expection of what the result is equal to.
@param [in] ... any additional function call that should execute on failure
that cannot be performed by the end test macro.

test_result and test_expected should be comparable with ==/!=. If a test
passes nothing happens. If a test fails output shows the failure. Upon
failure any cleanup function specified by the end test macro will execute
to prevent memory leaks. See the end test macro for more. If the test end
macro will not have access to some memory or operation in the scope of the
current check one may provide an additional function call they wish to have
executed on failure. This should only be used in special cases where such a
function is unusable by the end test macro due to scoping. */
#define CHECK(test_result, test_expected, ...)                                 \
    do                                                                         \
    {                                                                          \
        const __auto_type result_ = (test_result);                             \
        typeof(result_) const expected_ = (test_expected);                     \
        if (result_ != expected_)                                              \
        {                                                                      \
            TEST_PRINT_FAIL(result_, #test_result, expected_, #test_expected); \
            macro_test_res_ = FAIL;                                            \
            __VA_OPT__((void)__VA_ARGS__;)                                     \
            goto please_call_the_END_TEST_macro_at_the_end_of_this_test_;      \
        }                                                                      \
    } while (0)

/** @brief End every test started with the begin test macros.
@param [in] ... optional function call with any number of arguments that shall
be executed to clean up any allocated memory upon a test failure.
@return the test result from the currently executing test. FAIL upon the first
check failure or PASS if all checks have passed.

The call provided will be transposed directly as is. This also means that any
arguments provided to the cleanup function must be in the same scope as the
end test macro. Nested allocations within loops or if checks cannot be cleaned
up by this macro. Simpler tests and allocation strategies are therefore
recommended. */
#define END_TEST(...)                                                          \
please_call_the_END_TEST_macro_at_the_end_of_this_test_:                       \
    __VA_OPT__((void)__VA_ARGS__;)                                             \
    return macro_test_res_;                                                    \
    }                                                                          \
    while (0)

/** @brief Runs a list of test functions and returns the result.
@param [in] test_fn_list all test functions to run in main.

Return this macro from the main function of the test program. All tests
will run but the testing result for the entire program will be set to FAIL
upon the first failure. All functions must share the same signature, returning
a test_result and taking no arguments. */
#define RUN_TESTS(test_fn_list...)                                             \
    ({                                                                         \
        test_fn const all_tests_[] = {test_fn_list};                           \
        enum test_result all_tests_res_ = PASS;                                \
        for (unsigned i = 0; i < sizeof(all_tests_) / sizeof(test_fn); ++i)    \
        {                                                                      \
            if (all_tests_[i]() == FAIL)                                       \
            {                                                                  \
                all_tests_res_ = FAIL;                                         \
            }                                                                  \
        }                                                                      \
        all_tests_res_;                                                        \
    })

#endif /* CCC_TEST */
