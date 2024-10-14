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
    CHECK(is_empty(&fh), true);
    END_TEST();
}

Example with multiple parameters:

enum test_result insert_shuffled(ccc_ordered_multimap *,
                                 struct val[], size_t, int);

BEGIN_STATIC_TEST(insert_shuffled, ccc_ordered_multimap *pq,
                  struct val vals[], size_t const size, int const larger_prime)
{
    for (int i = 0 shuffled_index = larger_prime % size; i < size; ++i)
    {
        vals[shuffled_index].val = shuffled_index;
        push(pq, &vals[shuffled_index].elem);
        CHECK(ccc_omm_validate(pq), true);
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    END_TEST();
}*/
#define BEGIN_STATIC_TEST(test_name, ...)                                      \
    static enum test_result(test_name)(OPTIONAL_PARAMS(__VA_ARGS__))           \
    {                                                                          \
        enum test_result macro_test_res_ = PASS;

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
        enum test_result macro_test_res_ = PASS;

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
function is unusable by the end test macro due to scoping. Even then, consider
tracking allocations in an array of some sort to be cleaned up upon failure
or success in the end test macro. Function calls should be a semicolon
seperated list of calls with their appropriate args. If more complex code
needs to be written in the cleanup case a code block can be surrounded by
{...any code goes here...} braces for the formatting assistance braces provide,
though the braces are not required. */
#define CHECK(test_result, test_expected, ...)                                 \
    do                                                                         \
    {                                                                          \
        const __auto_type result_ = (test_result);                             \
        typeof(result_) const expected_ = (test_expected);                     \
        if (result_ != expected_)                                              \
        {                                                                      \
            TEST_PRINT_FAIL(result_, #test_result, expected_, #test_expected); \
            macro_test_res_ = FAIL;                                            \
            __VA_OPT__((void)({__VA_ARGS__});)                                 \
            goto use_at_least_one_check_and_finish_with_END_macro_call_;       \
        }                                                                      \
    } while (0)

/** Returns the current test status. If the end pass and end fail macros to
finish a test do not provide the necessary flow control for various test
scenarios, then check this status in the end test macro and act accordingly.

Example:
BEGIN_TEST(my_test)
{
    ...test code ommited...
    END_TEST({
        switch(TEST_STATUS)
        {
            case PASS:
                ...action...
            break;
            case FAIL:
                ...action...
            break;
            case ERROR:
                ...action...
            break;
        }
    });
}

This is not recommended as complex tests should be simplified such that one
of the end test macros is sufficient. */
#define TEST_STATUS macro_test_res_

/** @brief End every test started with the end test macro.
@param [in] ... optional code block with arbitrary cleanup code to be executed
to clean up any allocated memory upon test completion.
@return the test result from the currently executing test. FAIL upon the first
check failure or PASS if all checks have passed.

One may enter two braces and any code within them END_TEST({..code here...}).
This will help with formatting for more complicated cleanup procedures. This
end test macro is best to use when some code must execute every time a test
finishes whether it fails early or runs to completion. The code written will
execute whether all tests pass, or the first failure quits the test early.
Keep in mind that any function calls need to have access to variables from
earlier in the test if used. Standard scoping rules apply. This check operates
at function scope so does not have access to nested conditionals, loops, or
blocks from earlier in the test. See the check macro if more fine
grained control over nested scope is required upon a failure.*/
#define END_TEST(...)                                                          \
use_at_least_one_check_and_finish_with_END_macro_call_:                        \
    __VA_OPT__((void)({__VA_ARGS__});)                                         \
    return macro_test_res_;                                                    \
    }

/** @brief End every test started with the end pass macro.
@param [in] ... optional code block with arbitrary code to be executed only upon
all tests passing.
@return the test result from the currently executing test. FAIL upon the first
check failure or PASS if all checks have passed.

One may enter two braces and any code within them END_PASS({..code here...}).
This will help with formatting for more complicated cleanup procedures. This
end test macro is best to use when some code should only execute upon all
checks passing. Keep in mind that any function calls need to have access to
variables from earlier in the test if used. Standard scoping rules apply. This
check operates at function scope so does not have access to nested conditionals,
loops, or blocks from earlier in the test. See the check macro if more fine
grained control over nested scope is required upon a failure. */
#define END_PASS(...)                                                          \
use_at_least_one_check_and_finish_with_END_macro_call_:                        \
    __VA_OPT__((void)({                                                        \
                   if (macro_test_res_ == PASS)                                \
                   {                                                           \
                       __VA_ARGS__                                             \
                   }                                                           \
               });)                                                            \
    return macro_test_res_;                                                    \
    }

/** @brief End every test started with the end fail macros.
@param [in] ... optional code block with arbitrary cleanup code to be executed
only upon the first test failure.
@return the test result from the currently executing test. FAIL upon the first
check failure or PASS if all checks have passed.

One may enter two braces and any code within them END_FAIL({..code here...}).
This will help with formatting for more complicated cleanup procedures. This
end test macro is best to use when some code should only execute upon the first
test failure. Keep in mind that any function calls need to have access to
variables from earlier in the test if used. Standard scoping rules apply. This
check operates at function scope so does not have access to nested conditionals,
loops, or blocks from earlier in the test. See the check macro if more fine
grained control over nested scope is required upon a failure.*/
#define END_FAIL(...)                                                          \
use_at_least_one_check_and_finish_with_END_macro_call_:                        \
    __VA_OPT__((void)({                                                        \
                   if (macro_test_res_ == FAIL)                                \
                   {                                                           \
                       __VA_ARGS__                                             \
                   }                                                           \
               });)                                                            \
    return macro_test_res_;                                                    \
    }

/** @brief Runs a list of test functions and returns the result.
@param [in] test_fn_list all test functions to run in main.
@return a passing test result if all tests pass or failing result if any fail.
All tests to completion even if the overall result is a failure.

Return this macro from the main function of the test program. All tests
will run but the testing result for the entire program will be set to FAIL
upon the first failure. All functions must share the same signature, returning
a test_result and taking no arguments. */
#define RUN_TESTS(test_fn_list...)                                             \
    ({                                                                         \
        enum test_result const all_results_[] = {test_fn_list};                \
        enum test_result all_tests_res_ = PASS;                                \
        for (unsigned i = 0;                                                   \
             i < sizeof(all_results_) / sizeof(enum test_result); ++i)         \
        {                                                                      \
            if (all_results_[i] == FAIL)                                       \
            {                                                                  \
                all_tests_res_ = FAIL;                                         \
            }                                                                  \
        }                                                                      \
        all_tests_res_;                                                        \
    })

#endif /* CCC_TEST */
