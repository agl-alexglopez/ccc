#ifndef ATTRIB_H
#define ATTRIB_H

#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_LLVM_COMPILER)
#    if defined __has_attribute
#        if __has_attribute(deprecated)
#            define ATTRIB_PRIVATE __attribute__((deprecated("private")))
#        else
#            define ATTRIB_PRIVATE /**/
#        endif
#    else
#        define ATTRIB_PRIVATE /**/
#    endif
#else
#    define ATTRIB_PRIVATE /**/
#endif                     /* __GNUC__ || __clang__ || __INTEL_LLVM_COMPILER */

#endif
