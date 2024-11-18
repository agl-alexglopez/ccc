/* Author: Alexander G. Lopez */
#ifndef STR_VIEW
#define STR_VIEW

/* ATTRIB_PURE has no side effects when given the same arguments with the
   same data, producing the same return value. A str_view points to char
   const * data which may change in between str_view calls. ATTRIB_CONST
   applies only where no pointers are accessed or dereferenced. Other
   attributes provide more string safety and opportunities to optimize.
   Credit Harith on Code Review Stack Exchange. */
#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_LLVM_COMPILER)
#    if defined __has_attribute
#        if __has_attribute(pure)
#            define ATTRIB_PURE __attribute__((pure))
#        else
#            define ATTRIB_PURE /**/
#        endif
#        if __has_attribute(pure)
#            define ATTRIB_CONST __attribute__((const))
#        else
#            define ATTRIB_CONST /**/
#        endif
#        if __has_attribute(null_terminated_string_arg)
#            define ATTRIB_NULLTERM(...)                                       \
                __attribute__((null_terminated_string_arg(__VA_ARGS__)))
#        else
#            define ATTRIB_NULLTERM(...) /**/
#        endif
#    else
#        define ATTRIB_PURE          /**/
#        define ATTRIB_CONST         /**/
#        define ATTRIB_NULLTERM(...) /**/
#    endif
/* A helper macro to enforce only string literals for the SV constructor
   macro. GCC and Clang allow this syntax to create more errors when bad
   input is provided to the str_view SV constructor.*/
#    define STR_LITERAL(str) "" str ""
#else
#    define ATTRIB_PURE          /**/
#    define ATTRIB_CONST         /**/
#    define ATTRIB_NULLTERM(...) /**/
/* MSVC does not allow strong enforcement of string literals to the SV
   str_view constructor. This is a dummy wrapper for compatibility. */
#    define STR_LITERAL(str) str
#endif /* __GNUC__ || __clang__ || __INTEL_LLVM_COMPILER */

#if defined(_MSVC_VER) || defined(_WIN32) || defined(_WIN64)
#    if defined(SV_BUILD_DLL)
#        define SV_API __declspec(dllexport)
#    elif defined(SV_CONSUME_DLL)
#        define SV_API __declspec(dllimport)
#    else
#        define SV_API /**/
#    endif
#else
#    define SV_API /**/
#endif             /* _MSVC_VER */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/* A str_view is a read-only view of string data in C. It is modeled after
   the C++ std::string_view. It consists of a pointer to char const data
   and a size_t field. Therefore, the exact size of this type may be platform
   dependent but it is small enough that one should use the provided functions
   and pass by copy whenever possible. Avoid accessing struct fields. */
typedef struct
{
    char const *s;
    size_t len;
} str_view;

/* Standard three way comparison type in C. See the comparison
   functions for how to interpret the comparison results. ERR
   is returned if bad input is provided to any comparison. */
typedef enum
{
    SV_LES = -1,
    SV_EQL,
    SV_GRT,
    SV_ERR,
} sv_threeway_cmp;

/*==========================  Construction  ================================*/

/* A macro to reduce the chance for errors in repeating oneself when
   constructing an inline or const str_view. The input must be a string
   literal. For example:

      static str_view const prefix = SV("test_");

   One can even use this in code when string literals are used rather than
   saved constants to avoid errors in str_view constructions.

       for (str_view cur = sv_begin_tok(src, SV(" "));
            !sv_end_tok(src, cur);
            cur = sv_next_tok(src, cur, SV(" "))
       {}

   However saving the str_view in a constant may be more convenient. */
#define SV(str) ((str_view){STR_LITERAL(str), sizeof(str) - 1})

/* Constructs and returns a string view from a NULL TERMINATED string.
   It is undefined to construct a str_view from a non terminated string. */
SV_API str_view sv(char const *str) ATTRIB_NULLTERM(1) ATTRIB_PURE;

/* Constructs and returns a string view from a sequence of valid n bytes
   or string length, whichever comes first. The resulting str_view may
   or may not be null terminated at the index of its size. */
SV_API str_view sv_n(size_t n, char const *str) ATTRIB_NULLTERM(2) ATTRIB_PURE;

/* Constructs and returns a string view from a NULL TERMINATED string
   broken on the first ocurrence of delimeter if found or null
   terminator if delim cannot be found. This constructor will also
   skip the delimeter if that delimeter starts the string. This is similar
   to the tokenizing function in the iteration section. */
SV_API str_view sv_delim(char const *str, char const *delim) ATTRIB_NULLTERM(1)
    ATTRIB_NULLTERM(2) ATTRIB_PURE;

/* Returns the bytes of the string pointer to, null terminator included. */
SV_API size_t sv_strsize(char const *str) ATTRIB_NULLTERM(1) ATTRIB_PURE;

/* Copies the max of str_sz or src_str length into a view, whichever
   ends first. This is the same as sv_n. */
SV_API str_view sv_copy(size_t str_sz, char const *src_str)
    ATTRIB_NULLTERM(2) ATTRIB_PURE;

/* Fills the destination buffer with the minimum between
   destination size and source view size, null terminating
   the string. This may cut off src data if dest_sz < src.len.
   Returns how many bytes were written to the buffer. */
SV_API size_t sv_fill(size_t dest_sz, char *dest_buf, str_view src);

/* Returns the standard C threeway comparison between cmp(lhs, rhs)
   between a str_view and a c-string.
   str_view LES( -1  ) rhs (str_view is less than str)
   str_view EQL(  0  ) rhs (str_view is equal to str)
   str_view GRT(  1  ) rhs (str_view is greater than str)
   Comparison is bounded by the shorter str_view length. ERR is
   returned if bad input is provided such as a str_view with a
   NULL pointer field. */
SV_API sv_threeway_cmp sv_strcmp(str_view lhs, char const *rhs)
    ATTRIB_NULLTERM(2) ATTRIB_PURE;

/* Returns the standard C threeway comparison between cmp(lhs, rhs)
   between a str_view and the first n bytes (inclusive) of str
   or stops at the null terminator if that is encountered first.
   str_view LES( -1  ) rhs (str_view is less than str)
   str_view EQL(  0  ) rhs (str_view is equal to str)
   str_view GRT(  1  ) rhs (str_view is greater than str)
   Comparison is bounded by the shorter str_view length. ERR is
   returned if bad input is provided such as a str_view with a
   NULL pointer field. */
SV_API sv_threeway_cmp sv_strncmp(str_view lhs, char const *rhs, size_t n)
    ATTRIB_NULLTERM(2) ATTRIB_PURE;

/* Returns the minimum between the string size vs n bytes. */
SV_API size_t sv_minlen(char const *str, size_t n)
    ATTRIB_NULLTERM(1) ATTRIB_PURE;

/* Advances the pointer from its previous position. If NULL is provided
   sv_null() is returned. */
SV_API char const *sv_next(char const *c) ATTRIB_NULLTERM(1) ATTRIB_PURE;

/* Advances the iterator to the next character in the str_view
   being iterated through in reverse. It is undefined behavior
   to change the str_view one is iterating through during
   iteration. If the char pointer is null, sv_null() is returned. */
SV_API char const *sv_rnext(char const *c) ATTRIB_PURE;

/* Creates the substring from position pos for count length. The count is
   the minimum value between count and (length - pos). If an invalid
   position is given greater than str_view length an empty view is returned
   positioned at the end of str_view. This position may or may not hold the
   null terminator. */
SV_API str_view sv_substr(str_view sv, size_t pos, size_t count) ATTRIB_PURE;

/* A sentinel empty string. Safely dereferenced to view a null terminator.
   This may be returned from various functions when bad input is given
   such as NULL as the underlying str_view string pointer. */
SV_API char const *sv_null(void) ATTRIB_PURE;

/* The end of a str_view guaranteed to be greater than or equal to size.
   May be used for the idiomatic check for most string searching function
   return values when something is not found. If a size is returned from
   a searching function it is possible to check it against npos. */
SV_API size_t sv_npos(str_view sv) ATTRIB_CONST;

/* Returns true if the provided str_view is empty, false otherwise.
   This is a useful function to check for str_view searches that yield
   an empty view at the end of a str_view when an element cannot be
   found. */
SV_API bool sv_empty(str_view sv) ATTRIB_CONST;

/* Returns the length of the str_view in O(1) time. The position at
   str_view size is interpreted as the null terminator and not
   counted toward length of a str_view. */
SV_API size_t sv_len(str_view sv) ATTRIB_CONST;

/* Returns the bytes of str_view including null terminator. Note that
   string views may not actually be null terminated but the position at
   str_view[str_view.len] is interpreted as the null terminator and thus
   counts towards the byte count. */
SV_API size_t sv_size(str_view sv) ATTRIB_CONST;

/* Swaps the contents of a and b. Becuase these are read only views
   only pointers and sizes are exchanged. */
SV_API void sv_swap(str_view *a, str_view *b);

/* Returns a str_view of the entirety of the underlying string, starting
   at the current view pointer position. This guarantees that the str_view
   returned ends at the null terminator of the underlying string as all
   strings used with str_views are assumed to be null terminated. It is
   undefined behavior to provide non null terminated strings to any
   str_view code. */
SV_API str_view sv_extend(str_view sv) ATTRIB_PURE;

/* Returns the standard C threeway comparison between cmp(lhs, rhs)
   between two string views.
   lhs LES( -1  ) rhs (lhs is less than rhs)
   lhs EQL(  0  ) rhs (lhs is equal to rhs)
   lhs GRT(  1  ) rhs (lhs is greater than rhs).
   Comparison is bounded by the shorter str_view length. ERR is
   returned if bad input is provided such as a str_view with a
   NULL pointer field. */
SV_API sv_threeway_cmp sv_cmp(str_view lhs, str_view rhs) ATTRIB_PURE;

/* Finds the first tokenized position in the string view given any length
   delim str_view. Skips leading delimeters in construction. If the
   str_view to be searched stores NULL than the sv_null() is returned. If
   delim stores NULL, that is interpreted as a search for the null terminating
   character or empty string and the size zero substring at the final position
   in the str_view is returned wich may or may not be the null termiator. If no
   delim is found the entire str_view is returned. */
SV_API str_view sv_begin_tok(str_view src, str_view delim) ATTRIB_PURE;

/* Returns true if no further tokens are found and position is at the end
   position, meaning a call to sv_next_tok has yielded a size 0 str_view
   that points at the end of the src str_view which may or may not be null
   terminated. */
SV_API bool sv_end_tok(str_view src, str_view tok) ATTRIB_PURE;

/* Advances to the next token in the remaining view seperated by the delim.
   Repeating delimter patterns will be skipped until the next token or end
   of string is found. If str_view stores NULL the sv_null() placeholder
   is returned. If delim stores NULL the end position of the str_view
   is returned which may or may not be the null terminator. The tok is
   bounded by the length of the view between two delimeters or the length
   from a delimeter to the end of src, whichever comes first. */
SV_API str_view sv_next_tok(str_view src, str_view tok,
                            str_view delim) ATTRIB_PURE;

/* Obtains the last token in a string in preparation for reverse tokenized
   iteration. Any delimeters that end the string are skipped, as in the
   forward version. If src is NULL sv_null is returned. If delim is null
   the entire src view is returned. Though the str_view is tokenized in
   reverse, the token view will start at the first character and be the
   length of the token found. */
SV_API str_view sv_rbegin_tok(str_view src, str_view delim) ATTRIB_PURE;

/* Given the current str_view being iterated through and the current token
   in the iteration returns true if the ending state of a reverse tokenization
   has been reached, false otherwise. */
SV_API bool sv_rend_tok(str_view src, str_view tok) ATTRIB_PURE;

/* Advances the token in src to the next token between two delimeters provided
   by delim. Repeating delimiters are skipped until the next token is found.
   If no further tokens can be found an empty str_view is returned with its
   pointer set to the start of the src string being iterated through. Note
   that a multicharacter delimiter may yield different tokens in reverse
   than in the forward direction when partial matches occur and some portion
   of the delimeter is in a token. This is because the string is now being
   parsed from right to left. However, the token returned starts at the first
   character and is read from left to right between two delimeters as in the
   forward tokenization.  */
SV_API str_view sv_rnext_tok(str_view src, str_view tok,
                             str_view delim) ATTRIB_PURE;

/* Returns a read only pointer to the beginning of the string view,
   the first valid character in the view. If the view stores NULL,
   the placeholder sv_null() is returned. */
SV_API char const *sv_begin(str_view sv) ATTRIB_PURE;

/* Returns a read only pointer to the end of the string view. This
   may or may not be a null terminated character depending on the
   view. If the view stores NULL, the placeholder sv_null() is returned. */
SV_API char const *sv_end(str_view sv) ATTRIB_PURE;

/* Returns the reverse iterator beginning, the last character of the
   current view. If the view is null sv_null() is returned. If the
   view is sized zero with a valid pointer that pointer in the
   view is returned. */
SV_API char const *sv_rbegin(str_view sv) ATTRIB_PURE;

/* The ending position of a reverse iteration. It is undefined
   behavior to access or use rend. It is undefined behavior to
   pass in any str_view not being iterated through as started
   with rbegin. */
SV_API char const *sv_rend(str_view sv) ATTRIB_PURE;

/* Returns the character pointer at the minimum between the indicated
   position and the end of the string view. If NULL is stored by the
   str_view then sv_null() is returned. */
SV_API char const *sv_pos(str_view sv, size_t i) ATTRIB_PURE;

/* The characer in the string at position i with bounds checking.
   If i is greater than or equal to the size of str_view the null
   terminator character is returned. */
SV_API char sv_at(str_view sv, size_t i) ATTRIB_PURE;

/* The character at the first position of str_view. An empty
   str_view or NULL pointer is valid and will return '\0'. */
SV_API char sv_front(str_view sv) ATTRIB_PURE;

/* The character at the last position of str_view. An empty
   str_view or NULL pointer is valid and will return '\0'. */
SV_API char sv_back(str_view sv) ATTRIB_PURE;

/*============================  Searching  =================================*/

/* Searches for needle in hay starting from pos. If the needle
   is larger than the hay, or position is greater than hay length,
   then hay length is returned. */
SV_API size_t sv_find(str_view hay, size_t pos, str_view needle) ATTRIB_PURE;

/* Searches for the last occurence of needle in hay starting from pos
   from right to left. If found the starting position of the string
   is returned, the same as find. If not found hay size is returned.
   The only difference from find is the search direction. If needle
   is larger than hay, hay length is returned. If the position is
   larger than the hay, the entire hay is searched. */
SV_API size_t sv_rfind(str_view hay, size_t pos, str_view needle) ATTRIB_PURE;

/* Returns true if the needle is found in the hay, false otherwise. */
SV_API bool sv_contains(str_view hay, str_view needle) ATTRIB_PURE;

/* Returns a view of the needle found in hay at the first found
   position. If the needle cannot be found the empty view at the
   hay length position is returned. This may or may not be null
   terminated at that position. If needle is greater than
   hay length an empty view at the end of hay is returned. If
   hay is NULL, sv_null is returned (modeled after strstr). */
SV_API str_view sv_match(str_view hay, str_view needle) ATTRIB_PURE;

/* Returns a view of the needle found in hay at the last found
   position. If the needle cannot be found the empty view at the
   hay length position is returned. This may or may not be null
   terminated at that position. If needle is greater than
   hay length an empty view at hay size is returned. If hay is
   NULL, sv_null is returned (modeled after strstr). */
SV_API str_view sv_rmatch(str_view hay, str_view needle) ATTRIB_PURE;

/* Returns true if a prefix shorter than or equal in length to
   the str_view is present, false otherwise. */
SV_API bool sv_starts_with(str_view sv, str_view prefix) ATTRIB_PURE;

/* Removes the minimum between str_view length and n from the start
   of the str_view. It is safe to provide n larger than str_view
   size as that will result in a size 0 view to the end of the
   current view which may or may not be the null terminator. */
SV_API str_view sv_remove_prefix(str_view sv, size_t n) ATTRIB_PURE;

/* Returns true if a suffix less or equal in length to str_view is
   present, false otherwise. */
SV_API bool sv_ends_with(str_view sv, str_view suffix) ATTRIB_PURE;

/* Removes the minimum between str_view length and n from the end. It
   is safe to provide n larger than str_view and that will result in
   a size 0 view to the end of the current view which may or may not
   be the null terminator. */
SV_API str_view sv_remove_suffix(str_view sv, size_t n) ATTRIB_PURE;

/* Finds the first position of an occurence of any character in set.
   If no occurence is found hay size is returned. An empty set (NULL)
   is valid and will return position at hay size. An empty hay
   returns 0. */
SV_API size_t sv_find_first_of(str_view hay, str_view set) ATTRIB_PURE;

/* Finds the first position at which no characters in set can be found.
   If the string is all characters in set hay length is returned.
   An empty set (NULL) is valid and will return position 0. An empty
   hay returns 0. */
SV_API size_t sv_find_first_not_of(str_view hay, str_view set) ATTRIB_PURE;

/* Finds the last position of any character in set in hay. If
   no position is found hay size is returned. An empty set (NULL)
   is valid and returns hay size. An empty hay returns 0. */
SV_API size_t sv_find_last_of(str_view hay, str_view set) ATTRIB_PURE;

/* Finds the last position at which no character in set can be found.
   An empty set (NULL) is valid and will return the final character
   in the str_view. An empty hay will return 0. */
SV_API size_t sv_find_last_not_of(str_view hay, str_view set) ATTRIB_PURE;

/*============================  Printing  ==================================*/

/* Writes all characters in str_view to specified file such as stdout. */
SV_API void sv_print(FILE *f, str_view sv);

#endif /* STR_VIEW */
