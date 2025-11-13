/* Author: Alexander G. Lopez */
#ifndef STR_VIEW
#define STR_VIEW

/* ATTRIB_PURE has no side effects when given the same arguments with the
   same data, producing the same return value. A SV_String_view points to char
   const * data which may change in between SV_String_view calls. ATTRIB_CONST
   applies only where no pointers are accessed or dereferenced. Other
   attributes provide more string safety and opportunities to optimize.
   Credit Harith on Code Review Stack Exchange. */
#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_LLVM_COMPILER)
#    ifdef __has_attribute
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
   input is provided to the SV_String_view SV constructor.*/
#    define STR_LITERAL(str) "" str ""
#else
#    define ATTRIB_PURE          /**/
#    define ATTRIB_CONST         /**/
#    define ATTRIB_NULLTERM(...) /**/
/* MSVC does not allow strong enforcement of string literals to the SV
   SV_String_view constructor. This is a dummy wrapper for compatibility. */
#    define STR_LITERAL(str) str
#endif /* __GNUC__ || __clang__ || __INTEL_LLVM_COMPILER */

#if defined(_MSVC_VER) || defined(_WIN32) || defined(_WIN64)
#    ifdef SV_BUILD_DLL
#        define SV_API __declspec(dllexport)
#    elifdef SV_CONSUME_DLL
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

/* A SV_String_view is a read-only view of string data in C. It is modeled after
   the C++ std::string_view. It consists of a pointer to char const data
   and a size_t field. Therefore, the exact size of this type may be platform
   dependent but it is small enough that one should use the provided functions
   and pass by copy whenever possible. Avoid accessing struct fields. */
typedef struct
{
    char const *s;
    size_t len;
} SV_String_view;

/* Standard three way comparison type in C. See the comparison
   functions for how to interpret the comparison results. ERR
   is returned if bad input is provided to any comparison. */
typedef enum
{
    SV_LES = -1,
    SV_EQL,
    SV_GRT,
    SV_ERR,
} SV_Order;

/*==========================  Construction  ================================*/

/* A macro to reduce the chance for errors in repeating oneself when
   constructing an inline or const SV_String_view. The input must be a string
   literal. For example:

      static SV_String_view const prefix = SV("test_");

   One can even use this in code when string literals are used rather than
   saved constants to avoid errors in SV_String_view constructions.

       for (SV_String_view cur = SV_begin_tok(src, SV(" "));
            !SV_end_tok(src, cur);
            cur = SV_next_tok(src, cur, SV(" "))
       {}

   However saving the SV_String_view in a constant may be more convenient. */
#define SV(str) ((SV_String_view){STR_LITERAL(str), sizeof(str) - 1})

/* Constructs and returns a string view from a NULL TERMINATED string.
   It is undefined to construct a SV_String_view from a non terminated string.
 */
SV_API SV_String_view SV_sv(char const *str) ATTRIB_NULLTERM(1) ATTRIB_PURE;

/* Constructs and returns a string view from a sequence of valid n bytes
   or string length, whichever comes first. The resulting SV_String_view may
   or may not be null terminated at the index of its size. */
SV_API SV_String_view SV_sv_n(size_t n, char const *str)
    ATTRIB_NULLTERM(2) ATTRIB_PURE;

/* Constructs and returns a string view from a NULL TERMINATED string
   broken on the first ocurrence of delimeter if found or null
   terminator if delim cannot be found. This constructor will also
   skip the delimeter if that delimeter starts the string. This is similar
   to the tokenizing function in the iteration section. */
SV_API SV_String_view SV_delim(char const *str, char const *delim)
    ATTRIB_NULLTERM(1) ATTRIB_NULLTERM(2) ATTRIB_PURE;

/* Returns the bytes of the string pointer to, null terminator included. */
SV_API size_t SV_strsize(char const *str) ATTRIB_NULLTERM(1) ATTRIB_PURE;

/* Copies the max of str_sz or src_str length into a view, whichever
   ends first. This is the same as SV_n. */
SV_API SV_String_view SV_copy(size_t str_sz, char const *src_str)
    ATTRIB_NULLTERM(2) ATTRIB_PURE;

/* Fills the destination buffer with the minimum between
   destination size and source view size, null terminating
   the string. This may cut off src data if dest_sz < src.len.
   Returns how many bytes were written to the buffer. */
SV_API size_t SV_fill(size_t dest_sz, char *dest_buf, SV_String_view src);

/* Returns the standard C threeway comparison between cmp(lhs, rhs)
   between a SV_String_view and a c-string.
   SV_String_view LES( -1  ) rhs (SV_String_view is less than str)
   SV_String_view EQL(  0  ) rhs (SV_String_view is equal to str)
   SV_String_view GRT(  1  ) rhs (SV_String_view is greater than str)
   Comparison is bounded by the shorter SV_String_view length. ERR is
   returned if bad input is provided such as a SV_String_view with a
   NULL pointer field. */
SV_API SV_Order SV_strcmp(SV_String_view lhs, char const *rhs)
    ATTRIB_NULLTERM(2) ATTRIB_PURE;

/* Returns the standard C threeway comparison between cmp(lhs, rhs)
   between a SV_String_view and the first n bytes (inclusive) of str
   or stops at the null terminator if that is encountered first.
   SV_String_view LES( -1  ) rhs (SV_String_view is less than str)
   SV_String_view EQL(  0  ) rhs (SV_String_view is equal to str)
   SV_String_view GRT(  1  ) rhs (SV_String_view is greater than str)
   Comparison is bounded by the shorter SV_String_view length. ERR is
   returned if bad input is provided such as a SV_String_view with a
   NULL pointer field. */
SV_API SV_Order SV_strncmp(SV_String_view lhs, char const *rhs, size_t n)
    ATTRIB_NULLTERM(2) ATTRIB_PURE;

/* Returns the minimum between the string size vs n bytes. */
SV_API size_t SV_minlen(char const *str, size_t n)
    ATTRIB_NULLTERM(1) ATTRIB_PURE;

/* Advances the pointer from its previous position. If NULL is provided
   SV_null() is returned. */
SV_API char const *SV_next(char const *c) ATTRIB_NULLTERM(1) ATTRIB_PURE;

/* Advances the iterator to the next character in the SV_String_view
   being iterated through in reverse. It is undefined behavior
   to change the SV_String_view one is iterating through during
   iteration. If the char pointer is null, SV_null() is returned. */
SV_API char const *SV_rnext(char const *c) ATTRIB_PURE;

/* Creates the substring from position pos for count length. The count is
   the minimum value between count and (length - pos). If an invalid
   position is given greater than SV_String_view length an empty view is
   returned positioned at the end of SV_String_view. This position may or may
   not hold the null terminator. */
SV_API SV_String_view SV_substr(SV_String_view sv, size_t pos,
                                size_t count) ATTRIB_PURE;

/* A sentinel empty string. Safely dereferenced to view a null terminator.
   This may be returned from various functions when bad input is given
   such as NULL as the underlying SV_String_view string pointer. */
SV_API char const *SV_null(void) ATTRIB_PURE;

/* The end of a SV_String_view guaranteed to be greater than or equal to size.
   May be used for the idiomatic check for most string searching function
   return values when something is not found. If a size is returned from
   a searching function it is possible to check it against npos. */
SV_API size_t SV_npos(SV_String_view sv) ATTRIB_CONST;

/* Returns true if the provided SV_String_view is empty, false otherwise.
   This is a useful function to check for SV_String_view searches that yield
   an empty view at the end of a SV_String_view when an element cannot be
   found. */
SV_API bool SV_empty(SV_String_view sv) ATTRIB_CONST;

/* Returns the length of the SV_String_view in O(1) time. The position at
   SV_String_view size is interpreted as the null terminator and not
   counted toward length of a SV_String_view. */
SV_API size_t SV_len(SV_String_view sv) ATTRIB_CONST;

/* Returns the bytes of SV_String_view including null terminator. Note that
   string views may not actually be null terminated but the position at
   SV_String_view[SV_String_view.len] is interpreted as the null terminator and
   thus counts towards the byte count. */
SV_API size_t SV_size(SV_String_view sv) ATTRIB_CONST;

/* Swaps the contents of a and b. Becuase these are read only views
   only pointers and sizes are exchanged. */
SV_API void SV_swap(SV_String_view *a, SV_String_view *b);

/* Returns a SV_String_view of the entirety of the underlying string, starting
   at the current view pointer position. This guarantees that the SV_String_view
   returned ends at the null terminator of the underlying string as all
   strings used with SV_String_views are assumed to be null terminated. It is
   undefined behavior to provide non null terminated strings to any
   SV_String_view code. */
SV_API SV_String_view SV_extend(SV_String_view sv) ATTRIB_PURE;

/* Returns the standard C threeway comparison between cmp(lhs, rhs)
   between two string views.
   lhs LES( -1  ) rhs (lhs is less than rhs)
   lhs EQL(  0  ) rhs (lhs is equal to rhs)
   lhs GRT(  1  ) rhs (lhs is greater than rhs).
   Comparison is bounded by the shorter SV_String_view length. ERR is
   returned if bad input is provided such as a SV_String_view with a
   NULL pointer field. */
SV_API SV_Order SV_cmp(SV_String_view lhs, SV_String_view rhs) ATTRIB_PURE;

/* Finds the first tokenized position in the string view given any length
   delim SV_String_view. Skips leading delimeters in construction. If the
   SV_String_view to be searched stores NULL than the SV_null() is returned. If
   delim stores NULL, that is interpreted as a search for the null terminating
   character or empty string and the size zero substring at the final position
   in the SV_String_view is returned wich may or may not be the null termiator.
   If no delim is found the entire SV_String_view is returned. */
SV_API SV_String_view SV_begin_tok(SV_String_view src,
                                   SV_String_view delim) ATTRIB_PURE;

/* Returns true if no further tokens are found and position is at the end
   position, meaning a call to SV_next_tok has yielded a size 0 SV_String_view
   that points at the end of the src SV_String_view which may or may not be null
   terminated. */
SV_API bool SV_end_tok(SV_String_view src, SV_String_view tok) ATTRIB_PURE;

/* Advances to the next token in the remaining view seperated by the delim.
   Repeating delimter patterns will be skipped until the next token or end
   of string is found. If SV_String_view stores NULL the SV_null() placeholder
   is returned. If delim stores NULL the end position of the SV_String_view
   is returned which may or may not be the null terminator. The tok is
   bounded by the length of the view between two delimeters or the length
   from a delimeter to the end of src, whichever comes first. */
SV_API SV_String_view SV_next_tok(SV_String_view src, SV_String_view tok,
                                  SV_String_view delim) ATTRIB_PURE;

/* Obtains the last token in a string in preparation for reverse tokenized
   iteration. Any delimeters that end the string are skipped, as in the
   forward version. If src is NULL SV_null is returned. If delim is null
   the entire src view is returned. Though the SV_String_view is tokenized in
   reverse, the token view will start at the first character and be the
   length of the token found. */
SV_API SV_String_view SV_rbegin_tok(SV_String_view src,
                                    SV_String_view delim) ATTRIB_PURE;

/* Given the current SV_String_view being iterated through and the current token
   in the iteration returns true if the ending state of a reverse tokenization
   has been reached, false otherwise. */
SV_API bool SV_rend_tok(SV_String_view src, SV_String_view tok) ATTRIB_PURE;

/* Advances the token in src to the next token between two delimeters provided
   by delim. Repeating delimiters are skipped until the next token is found.
   If no further tokens can be found an empty SV_String_view is returned with
   its pointer set to the start of the src string being iterated through. Note
   that a multicharacter delimiter may yield different tokens in reverse
   than in the forward direction when partial matches occur and some portion
   of the delimeter is in a token. This is because the string is now being
   parsed from right to left. However, the token returned starts at the first
   character and is read from left to right between two delimeters as in the
   forward tokenization.  */
SV_API SV_String_view SV_rnext_tok(SV_String_view src, SV_String_view tok,
                                   SV_String_view delim) ATTRIB_PURE;

/* Returns a read only pointer to the beginning of the string view,
   the first valid character in the view. If the view stores NULL,
   the placeholder SV_null() is returned. */
SV_API char const *SV_begin(SV_String_view sv) ATTRIB_PURE;

/* Returns a read only pointer to the end of the string view. This
   may or may not be a null terminated character depending on the
   view. If the view stores NULL, the placeholder SV_null() is returned. */
SV_API char const *SV_end(SV_String_view sv) ATTRIB_PURE;

/* Returns the reverse iterator beginning, the last character of the
   current view. If the view is null SV_null() is returned. If the
   view is sized zero with a valid pointer that pointer in the
   view is returned. */
SV_API char const *SV_rbegin(SV_String_view sv) ATTRIB_PURE;

/* The ending position of a reverse iteration. It is undefined
   behavior to access or use rend. It is undefined behavior to
   pass in any SV_String_view not being iterated through as started
   with rbegin. */
SV_API char const *SV_rend(SV_String_view sv) ATTRIB_PURE;

/* Returns the character pointer at the minimum between the indicated
   position and the end of the string view. If NULL is stored by the
   SV_String_view then SV_null() is returned. */
SV_API char const *SV_pos(SV_String_view sv, size_t i) ATTRIB_PURE;

/* The characer in the string at position i with bounds checking.
   If i is greater than or equal to the size of SV_String_view the null
   terminator character is returned. */
SV_API char SV_at(SV_String_view sv, size_t i) ATTRIB_PURE;

/* The character at the first position of SV_String_view. An empty
   SV_String_view or NULL pointer is valid and will return '\0'. */
SV_API char SV_front(SV_String_view sv) ATTRIB_PURE;

/* The character at the last position of SV_String_view. An empty
   SV_String_view or NULL pointer is valid and will return '\0'. */
SV_API char SV_back(SV_String_view sv) ATTRIB_PURE;

/*============================  Searching  =================================*/

/* Searches for needle in hay starting from pos. If the needle
   is larger than the hay, or position is greater than hay length,
   then hay length is returned. */
SV_API size_t SV_find(SV_String_view hay, size_t pos,
                      SV_String_view needle) ATTRIB_PURE;

/* Searches for the last occurence of needle in hay starting from pos
   from right to left. If found the starting position of the string
   is returned, the same as find. If not found hay size is returned.
   The only difference from find is the search direction. If needle
   is larger than hay, hay length is returned. If the position is
   larger than the hay, the entire hay is searched. */
SV_API size_t SV_rfind(SV_String_view hay, size_t pos,
                       SV_String_view needle) ATTRIB_PURE;

/* Returns true if the needle is found in the hay, false otherwise. */
SV_API bool SV_contains(SV_String_view hay, SV_String_view needle) ATTRIB_PURE;

/* Returns a view of the needle found in hay at the first found
   position. If the needle cannot be found the empty view at the
   hay length position is returned. This may or may not be null
   terminated at that position. If needle is greater than
   hay length an empty view at the end of hay is returned. If
   hay is NULL, SV_null is returned (modeled after strstr). */
SV_API SV_String_view SV_match(SV_String_view hay,
                               SV_String_view needle) ATTRIB_PURE;

/* Returns a view of the needle found in hay at the last found
   position. If the needle cannot be found the empty view at the
   hay length position is returned. This may or may not be null
   terminated at that position. If needle is greater than
   hay length an empty view at hay size is returned. If hay is
   NULL, SV_null is returned (modeled after strstr). */
SV_API SV_String_view SV_rmatch(SV_String_view hay,
                                SV_String_view needle) ATTRIB_PURE;

/* Returns true if a prefix shorter than or equal in length to
   the SV_String_view is present, false otherwise. */
SV_API bool SV_starts_with(SV_String_view sv,
                           SV_String_view prefix) ATTRIB_PURE;

/* Removes the minimum between SV_String_view length and n from the start
   of the SV_String_view. It is safe to provide n larger than SV_String_view
   size as that will result in a size 0 view to the end of the
   current view which may or may not be the null terminator. */
SV_API SV_String_view SV_remove_prefix(SV_String_view sv, size_t n) ATTRIB_PURE;

/* Returns true if a suffix less or equal in length to SV_String_view is
   present, false otherwise. */
SV_API bool SV_ends_with(SV_String_view sv, SV_String_view suffix) ATTRIB_PURE;

/* Removes the minimum between SV_String_view length and n from the end. It
   is safe to provide n larger than SV_String_view and that will result in
   a size 0 view to the end of the current view which may or may not
   be the null terminator. */
SV_API SV_String_view SV_remove_suffix(SV_String_view sv, size_t n) ATTRIB_PURE;

/* Finds the first position of an occurence of any character in set.
   If no occurence is found hay size is returned. An empty set (NULL)
   is valid and will return position at hay size. An empty hay
   returns 0. */
SV_API size_t SV_find_first_of(SV_String_view hay,
                               SV_String_view set) ATTRIB_PURE;

/* Finds the first position at which no characters in set can be found.
   If the string is all characters in set hay length is returned.
   An empty set (NULL) is valid and will return position 0. An empty
   hay returns 0. */
SV_API size_t SV_find_first_not_of(SV_String_view hay,
                                   SV_String_view set) ATTRIB_PURE;

/* Finds the last position of any character in set in hay. If
   no position is found hay size is returned. An empty set (NULL)
   is valid and returns hay size. An empty hay returns 0. */
SV_API size_t SV_find_last_of(SV_String_view hay,
                              SV_String_view set) ATTRIB_PURE;

/* Finds the last position at which no character in set can be found.
   An empty set (NULL) is valid and will return the final character
   in the SV_String_view. An empty hay will return 0. */
SV_API size_t SV_find_last_not_of(SV_String_view hay,
                                  SV_String_view set) ATTRIB_PURE;

/*============================  Printing  ==================================*/

/* Writes all characters in SV_String_view to specified file such as stdout. */
SV_API void SV_print(FILE *f, SV_String_view sv);

#endif /* STR_VIEW */
