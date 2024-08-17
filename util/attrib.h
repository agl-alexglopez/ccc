#ifndef CCC_ATTRIB_H
#define CCC_ATTRIB_H

#include <stdio.h>
#include <stdlib.h>

#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_LLVM_COMPILER)
#    if defined __has_attribute
#        if __has_attribute(deprecated)
#            define ATTRIB_PRIVATE __attribute__((deprecated("private")))
#        else
#            define ATTRIB_PRIVATE /**/
#        endif
#        if __has_attribute(nonnull)
#            define ATTRIB_NONNULL(...) __attribute__((nonnull(__VA_ARGS__)))
#        else
#            define ATTRIB_NONNULL(...) /**/
#        endif
#    else
#        define ATTRIB_PRIVATE      /**/
#        define ATTRIB_NONNULL(...) /**/
#    endif
#else
#    define ATTRIB_PRIVATE /**/
#endif                     /* __GNUC__ || __clang__ || __INTEL_LLVM_COMPILER */

#define UNIMPLEMENTED()                                                        \
    do                                                                         \
    {                                                                          \
        (void)fprintf(stderr,                                                  \
                      "\n!!Line: %d, File: %s. Func %s not implemented\n",     \
                      __LINE__, __FILE__, __func__);                           \
        exit(EXIT_FAILURE);                                                    \
    } while (0)

#endif /* CCC_ATTRIB_H */
