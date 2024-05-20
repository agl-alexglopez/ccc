/* Author: Alexander G. Lopez
   File: str_view.c
   ===================
   This file implements the str_view interface as an approximation of C++
   std::string_view type. There are some minor differences and C flavor thrown
   in. Additionally, there is a provided reimplementation of the Two-Way
   String-Searching algorithm, similar to glibc. */
#include "str_view.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* ========================   Type Definitions   =========================== */

/* Return type for the factorization step of two-way search. */
struct sv_factorization
{
    /* Position in the needle at which (local period = period). */
    ssize_t start_critical_pos;
    /* A distance in the needle such that two letters always coincide. */
    ssize_t period_dist;
};

struct sv_two_way_pack
{
    const char *const hay;
    ssize_t hay_sz;
    const char *const needle;
    ssize_t needle_sz;
    /* Taken from the factorization of needle for two-way searching */
    ssize_t period_dist;
    /* The critical position taken from our factorization. */
    ssize_t critical_pos;
};

/* Avoid giving the user a chance to dereference null as much as posssible
   by returning this for various edgecases when it makes sense to communicate
   empty, null, invalid, not found etc. Used on cases by case basis.
   It is usually better to justify giving back the user pointer in a
   str_view even if it sized 0 and pointing to null terminator. */
static const str_view nil = SV("");

/* =========================   Prototypes   =============================== */

static size_t sv_after_find(str_view, str_view);
static size_t sv_before_rfind(str_view, str_view);
static size_t sv_min(size_t, size_t);
static size_t sv_two_way(const char *, ssize_t, const char *, ssize_t);
static size_t sv_rtwo_way(const char *, ssize_t, const char *, ssize_t);
static struct sv_factorization sv_maximal_suffix(const char *, ssize_t);
static struct sv_factorization sv_maximal_suffix_rev(const char *, ssize_t);
static struct sv_factorization sv_rmaximal_suffix(const char *, ssize_t);
static struct sv_factorization sv_rmaximal_suffix_rev(const char *, ssize_t);
static size_t sv_two_way_memoization(struct sv_two_way_pack);
static size_t sv_two_way_normal(struct sv_two_way_pack);
static size_t sv_rtwo_way_memoization(struct sv_two_way_pack);
static size_t sv_rtwo_way_normal(struct sv_two_way_pack);
static sv_threeway_cmp sv_char_cmp(char, char);
static ssize_t sv_ssizet_max(ssize_t, ssize_t);
static size_t sv_twobyte_strnstrn(const unsigned char *, size_t,
                                  const unsigned char *);
static size_t sv_threebyte_strnstrn(const unsigned char *, size_t,
                                    const unsigned char *);
static size_t sv_fourbyte_strnstrn(const unsigned char *, size_t,
                                   const unsigned char *);
static size_t sv_strcspn(const char *, size_t, const char *, size_t);
static size_t sv_strspn(const char *, size_t, const char *, size_t);
static size_t sv_strnstrn(const char *, ssize_t, const char *, ssize_t);
static size_t sv_strnchr(const char *, char, size_t);
static size_t sv_rstrnchr(const char *, char, size_t);
static size_t sv_rstrnstrn(const char *, ssize_t, const char *, ssize_t);
static size_t sv_rtwobyte_strnstrn(const unsigned char *, size_t,
                                   const unsigned char *);
static size_t sv_rthreebyte_strnstrn(const unsigned char *, size_t,
                                     const unsigned char *);
static size_t sv_rfourbyte_strnstrn(const unsigned char *, size_t,
                                    const unsigned char *);

/* ===================   Interface Implementation   ====================== */

str_view
sv(const char str[static const 1])
{
    if (!str)
    {
        return nil;
    }
    return (str_view){.s = str, .sz = strlen(str)};
}

str_view
sv_n(size_t n, const char str[static const 1])
{
    if (!str || !n)
    {
        return nil;
    }
    return (str_view){.s = str, .sz = strnlen(str, n)};
}

str_view
sv_delim(const char str[static const 1], const char delim[static const 1])
{
    if (!str)
    {
        return nil;
    }
    if (!delim)
    {
        return (str_view){.s = str, .sz = strlen(str)};
    }
    return sv_begin_tok(
        (str_view){
            .s = str,
            .sz = strlen(str),
        },
        (str_view){
            .s = delim,
            .sz = strlen(delim),
        });
}

void
sv_print(FILE *f, str_view sv)
{
    if (!sv.s || nil.s == sv.s || !sv.sz || !f)
    {
        return;
    }
    /* printf does not output the null terminator in normal strings so
       as long as we output correct number of characters we do the same */
    (void)fwrite(sv.s, sizeof(char), sv.sz, f);
}

str_view
sv_copy(const size_t str_sz, const char src_str[static const 1])
{
    return sv_n(str_sz, src_str);
}

size_t
sv_fill(size_t dest_sz, char dest_buf[static const dest_sz], str_view src)
{
    if (!dest_buf || !dest_sz || !src.s || !src.sz)
    {
        return 0;
    }
    const size_t bytes = sv_min(dest_sz, sv_size(src));
    memmove(dest_buf, src.s, bytes);
    dest_buf[bytes - 1] = '\0';
    return bytes;
}

bool
sv_empty(const str_view sv)
{
    return !sv.s || !sv.sz;
}

size_t
sv_len(str_view sv)
{
    return sv.sz;
}

size_t
sv_size(str_view sv)
{
    return sv.sz + 1;
}

size_t
sv_strsize(const char str[static const 1])
{
    if (!str)
    {
        return 0;
    }
    return strlen(str) + 1;
}

size_t
sv_minlen(const char str[static const 1], size_t n)
{
    if (!str)
    {
        return 0;
    }
    return strnlen(str, n);
}

char
sv_at(str_view sv, size_t i)
{
    if (i >= sv.sz)
    {
        return *nil.s;
    }
    return sv.s[i];
}

const char *
sv_null(void)
{
    return nil.s;
}

void
sv_swap(str_view *a, str_view *b)
{
    if (a == b || !a || !b)
    {
        return;
    }
    const str_view tmp_b = (str_view){.s = b->s, .sz = b->sz};
    b->s = a->s;
    b->sz = a->sz;
    a->s = tmp_b.s;
    a->sz = tmp_b.sz;
}

sv_threeway_cmp
sv_cmp(str_view lhs, str_view rhs)
{
    if (!lhs.s || !rhs.s)
    {
        return SV_ERR;
    }
    const size_t sz = sv_min(lhs.sz, rhs.sz);
    size_t i = 0;
    for (; i < sz && lhs.s[i] == rhs.s[i]; ++i)
    {}
    if (i == lhs.sz && i == rhs.sz)
    {
        return SV_EQL;
    }
    if (i < lhs.sz && i < rhs.sz)
    {
        return (uint8_t)lhs.s[i] < (uint8_t)rhs.s[i] ? SV_LES : SV_GRT;
    }
    return (i < lhs.sz) ? SV_GRT : SV_LES;
}

sv_threeway_cmp
sv_strcmp(str_view lhs, const char rhs[static const 1])
{
    if (!lhs.s || !rhs)
    {
        return SV_ERR;
    }
    const size_t sz = lhs.sz;
    size_t i = 0;
    for (; i < sz && rhs[i] != '\0' && lhs.s[i] == rhs[i]; ++i)
    {}
    if (i == lhs.sz && rhs[i] == '\0')
    {
        return SV_EQL;
    }
    if (i < lhs.sz && rhs[i] != '\0')
    {
        return (uint8_t)lhs.s[i] < (uint8_t)rhs[i] ? SV_LES : SV_GRT;
    }
    return (i < lhs.sz) ? SV_GRT : SV_LES;
}

sv_threeway_cmp
sv_strncmp(str_view lhs, const char rhs[static const 1], const size_t n)
{
    if (!lhs.s || !rhs)
    {
        return SV_ERR;
    }
    const size_t sz = sv_min(lhs.sz, n);
    size_t i = 0;
    for (; i < sz && rhs[i] != '\0' && lhs.s[i] == rhs[i]; ++i)
    {}
    if (i == lhs.sz && sz == n)
    {
        return SV_EQL;
    }
    /* strncmp compares the first at most n bytes inclusive */
    if (i < lhs.sz && sz <= n)
    {
        return (uint8_t)lhs.s[i] < (uint8_t)rhs[i] ? SV_LES : SV_GRT;
    }
    return (i < lhs.sz) ? SV_GRT : SV_LES;
}

char
sv_front(str_view sv)
{
    if (!sv.s || !sv.sz)
    {
        return *nil.s;
    }
    return *sv.s;
}

char
sv_back(str_view sv)
{
    if (!sv.s || !sv.sz)
    {
        return *nil.s;
    }
    return sv.s[sv.sz - 1];
}

const char *
sv_begin(const str_view sv)
{
    if (!sv.s)
    {
        return nil.s;
    }
    return sv.s;
}

const char *
sv_end(const str_view sv)
{
    if (!sv.s || sv.s == nil.s)
    {
        return nil.s;
    }
    return sv.s + sv.sz;
}

const char *
sv_next(const char c[static 1])
{
    if (!c)
    {
        return nil.s;
    }
    return ++c;
}

const char *
sv_rbegin(str_view sv)
{
    if (!sv.s)
    {
        return nil.s;
    }
    if (!sv.sz)
    {
        return sv.s;
    }
    return sv.s + sv.sz - 1;
}

const char *
sv_rend(str_view sv)
{
    if (!sv.s || sv.s == nil.s)
    {
        return nil.s;
    }
    if (!sv.sz)
    {
        return sv.s;
    }
    return sv.s - 1;
}

const char *
sv_rnext(const char c[static 1])
{
    if (!c)
    {
        return nil.s;
    }
    return --c;
}

const char *
sv_pos(str_view sv, size_t i)
{
    if (!sv.s)
    {
        return nil.s;
    }
    if (i > sv.sz)
    {
        return sv_end(sv);
    }
    return sv.s + i;
}

str_view
sv_begin_tok(str_view src, str_view delim)
{
    if (!src.s)
    {
        return nil;
    }
    if (!delim.s)
    {
        return (str_view){.s = src.s + src.sz, 0};
    }
    const char *const begin = src.s;
    const size_t sv_not = sv_after_find(src, delim);
    src.s += sv_not;
    if (begin + src.sz == src.s)
    {
        return (str_view){.s = src.s, .sz = 0};
    }
    src.sz -= sv_not;
    return (str_view){.s = src.s, .sz = sv_find(src, 0, delim)};
}

bool
sv_end_tok(const str_view src, const str_view tok)
{
    return !tok.sz || tok.s >= (src.s + src.sz);
}

str_view
sv_next_tok(const str_view src, str_view tok, str_view delim)
{
    if (!tok.s)
    {
        return nil;
    }
    if (!delim.s || !tok.s || tok.s[tok.sz] == '\0')
    {
        return (str_view){.s = tok.s + tok.sz, .sz = 0};
    }
    str_view next = {.s = tok.s + tok.sz, .sz = src.sz - tok.sz};
    if (next.s >= src.s + src.sz)
    {
        return (str_view){.s = src.s + src.sz, .sz = 0};
    }
    next.s += delim.sz;
    next.sz = src.sz - (next.s - src.s);
    /* There is a cheap easy way to skip repeating delimiters before the
       next search that should be faster than string comparison. */
    const size_t after_delim = sv_after_find(next, delim);
    next.s += after_delim;
    next.sz -= after_delim;
    if (next.s >= src.s + src.sz)
    {
        return (str_view){.s = src.s + src.sz, .sz = 0};
    }
    const size_t found
        = sv_strnstrn(next.s, (ssize_t)next.sz, delim.s, (ssize_t)delim.sz);
    return (str_view){.s = next.s, .sz = found};
}

str_view
sv_rbegin_tok(str_view src, str_view delim)
{
    if (!src.s)
    {
        return nil;
    }
    if (!delim.s)
    {
        return (str_view){.s = src.s + src.sz, 0};
    }
    size_t before_delim = sv_before_rfind(src, delim);
    src.sz = sv_min(src.sz, before_delim + 1);
    size_t start = sv_rfind(src, src.sz, delim);
    if (start == src.sz)
    {
        return src;
    }
    start += delim.sz;
    return (str_view){.s = src.s + start, .sz = before_delim - start + 1};
}

str_view
sv_rnext_tok(const str_view src, str_view tok, str_view delim)
{
    if (!tok.s)
    {
        return nil;
    }
    if (!tok.sz | !delim.s || tok.s == src.s || tok.s - delim.sz <= src.s)
    {
        return (str_view){.s = src.s, .sz = 0};
    }
    const str_view shorter = {.s = src.s, .sz = (tok.s - delim.sz) - src.s};
    /* Same as in the forward version, this method is a quick way to skip
       any number of repeating delimiters before starting the next search
       for a delimiter before a token. */
    const size_t before_delim = sv_before_rfind(shorter, delim);
    if (before_delim == shorter.sz)
    {
        return shorter;
    }
    size_t start = sv_rstrnstrn(shorter.s, (ssize_t)before_delim, delim.s,
                                (ssize_t)delim.sz);
    if (start == before_delim)
    {
        return (str_view){.s = shorter.s, .sz = before_delim + 1};
    }
    start += delim.sz;
    return (str_view){.s = src.s + start, .sz = before_delim - start + 1};
}

bool
sv_rend_tok(const str_view src, const str_view tok)
{
    return !tok.sz && tok.s == src.s;
}

str_view
sv_extend(str_view sv)
{
    if (!sv.s)
    {
        return nil;
    }
    const char *i = sv.s;
    while (*i++)
    {}
    sv.sz = i - sv.s - 1;
    return sv;
}

bool
sv_starts_with(str_view sv, str_view prefix)
{
    if (prefix.sz > sv.sz)
    {
        return false;
    }
    return sv_cmp(sv_substr(sv, 0, prefix.sz), prefix) == SV_EQL;
}

str_view
sv_remove_prefix(const str_view sv, const size_t n)
{
    const size_t remove = sv_min(sv.sz, n);
    return (str_view){.s = sv.s + remove, .sz = sv.sz - remove};
}

bool
sv_ends_with(str_view sv, str_view suffix)
{
    if (suffix.sz > sv.sz)
    {
        return false;
    }
    return sv_cmp(sv_substr(sv, sv.sz - suffix.sz, suffix.sz), suffix)
           == SV_EQL;
}

str_view
sv_remove_suffix(const str_view sv, const size_t n)
{
    if (!sv.s)
    {
        return nil;
    }
    return (str_view){.s = sv.s, .sz = sv.sz - sv_min(sv.sz, n)};
}

str_view
sv_substr(str_view sv, size_t pos, size_t count)
{
    if (pos > sv.sz)
    {
        return (str_view){.s = sv.s + sv.sz, .sz = 0};
    }
    return (str_view){.s = sv.s + pos, .sz = sv_min(count, sv.sz - pos)};
}

bool
sv_contains(str_view hay, str_view needle)
{
    if (needle.sz > hay.sz)
    {
        return false;
    }
    if (sv_empty(hay))
    {
        return false;
    }
    if (sv_empty(needle))
    {
        return true;
    }
    return hay.sz
           != sv_strnstrn(hay.s, (ssize_t)hay.sz, needle.s, (ssize_t)needle.sz);
}

str_view
sv_match(str_view hay, str_view needle)
{
    if (!hay.s || !needle.s)
    {
        return nil;
    }
    if (needle.sz > hay.sz || sv_empty(hay) || sv_empty(needle))
    {
        return (str_view){.s = hay.s + hay.sz, .sz = 0};
    }
    const size_t found
        = sv_strnstrn(hay.s, (ssize_t)hay.sz, needle.s, (ssize_t)needle.sz);
    return found == hay.sz ? (str_view){.s = hay.s + hay.sz, .sz = 0}
                           : (str_view){.s = hay.s + found, .sz = needle.sz};
}

str_view
sv_rmatch(str_view hay, str_view needle)
{
    if (!hay.s)
    {
        return nil;
    }
    if (sv_empty(hay) || sv_empty(needle))
    {
        return (str_view){.s = hay.s + hay.sz, .sz = 0};
    }
    const size_t found
        = sv_rstrnstrn(hay.s, (ssize_t)hay.sz, needle.s, (ssize_t)needle.sz);
    return found == hay.sz ? (str_view){.s = hay.s + hay.sz, .sz = 0}
                           : (str_view){.s = hay.s + found, .sz = needle.sz};
}

size_t
sv_find(str_view hay, size_t pos, str_view needle)
{
    if (needle.sz > hay.sz || pos > hay.sz)
    {
        return hay.sz;
    }
    return pos
           + sv_strnstrn(hay.s + pos, (ssize_t)(hay.sz - pos), needle.s,
                         (ssize_t)needle.sz);
}

size_t
sv_rfind(str_view h, size_t pos, str_view n)
{
    if (!h.sz || n.sz > h.sz)
    {
        return h.sz;
    }
    if (pos >= h.sz)
    {
        pos = h.sz - 1;
    }
    const size_t found
        = sv_rstrnstrn(h.s, (ssize_t)pos + 1, n.s, (ssize_t)n.sz);
    return found == pos + 1 ? h.sz : found;
}

size_t
sv_find_first_of(str_view hay, str_view set)
{
    if (!hay.s || !hay.sz)
    {
        return 0;
    }
    if (!set.s || !set.sz)
    {
        return hay.sz;
    }
    return sv_strcspn(hay.s, hay.sz, set.s, set.sz);
}

size_t
sv_find_last_of(str_view hay, str_view set)
{
    if (!hay.s || !hay.sz)
    {
        return 0;
    }
    if (!set.s || !set.sz)
    {
        return hay.sz;
    }
    size_t last_pos = hay.sz;
    for (size_t in = 0, prev = 0;
         (in += sv_strspn(hay.s + in, hay.sz - in, set.s, set.sz)) != hay.sz;
         ++in, prev = in)
    {
        if (in != prev)
        {
            last_pos = in - 1;
        }
    }
    return last_pos;
}

size_t
sv_find_first_not_of(str_view hay, str_view set)
{
    if (!hay.s || !hay.sz)
    {
        return 0;
    }
    if (!set.s || !set.sz)
    {
        return 0;
    }
    return sv_strspn(hay.s, hay.sz, set.s, set.sz);
}

size_t
sv_find_last_not_of(str_view hay, str_view set)
{
    if (!hay.s || !hay.sz)
    {
        return 0;
    }
    if (!set.s || !set.sz)
    {
        return hay.sz - 1;
    }
    size_t last_pos = hay.sz;
    for (size_t in = 0, prev = 0;
         (in += sv_strspn(hay.s + in, hay.sz - in, set.s, set.sz)) != hay.sz;
         ++in, prev = in)
    {
        if (in != prev)
        {
            last_pos = in;
        }
    }
    return last_pos;
}

size_t
sv_npos(str_view sv)
{
    return sv.sz;
}

/* ======================   Static Helpers    ============================= */

static size_t
sv_after_find(str_view hay, str_view needle)
{
    if (needle.sz > hay.sz)
    {
        return 0;
    }
    size_t delim_i = 0;
    size_t i = 0;
    for (; i < hay.sz && needle.s[delim_i] == hay.s[i]; ++i)
    {
        delim_i = (delim_i + 1) % needle.sz;
    }
    /* Also reset to the last mismatch found. If some of the delimeter matched
       but then the string changed into a mismatch go back to get characters
       that are partially in the delimeter. */
    return i - delim_i;
}

static size_t
sv_before_rfind(str_view hay, str_view needle)
{
    if (needle.sz > hay.sz || !needle.sz || !hay.sz)
    {
        return hay.sz;
    }
    size_t delim_i = 0;
    size_t i = 0;
    for (; i < hay.sz
           && needle.s[needle.sz - delim_i - 1] == hay.s[hay.sz - i - 1];
         ++i)
    {
        delim_i = (delim_i + 1) % needle.sz;
    }
    /* Ugly logic to account for the reverse nature of this modulo search.
       the position needs to account for any part of the delim that may
       have started to match but then mismatched. The 1 is because
       this in an index being returned not a length. */
    return i == hay.sz ? hay.sz : hay.sz - i + delim_i - 1;
}

static inline size_t
sv_min(size_t a, size_t b)
{
    return a < b ? a : b;
}

static inline ssize_t
sv_ssizet_max(ssize_t a, ssize_t b)
{
    return a > b ? a : b;
}

static inline sv_threeway_cmp
sv_char_cmp(char a, char b)
{
    return (a > b) - (a < b);
}

/* ======================   Static Utilities    =========================== */

/* This is section is modeled after the musl string.h library. However,
   using str_view that may not be null terminated requires modifications. */

#define BITOP(a, b, op)                                                        \
    ((a)[(size_t)(b) / (8 * sizeof *(a))] op(size_t) 1                         \
     << ((size_t)(b) % (8 * sizeof *(a))))

/* This is dangerous. Do not use this under normal circumstances.
   This is an internal helper for the backwards two way string
   searching algorithm. It expects that both arguments are
   greater than or equal to n bytes in length similar to how
   the forward version expects the same. However, the comparison
   moves backward from the location provided for n bytes. */
static int
sv_rmemcmp(const void *const vl, const void *const vr, size_t n)
{
    const unsigned char *l = vl;
    const unsigned char *r = vr;
    for (; n && *l == *r; n--, l--, r--)
    {}
    return n ? *l - *r : 0;
}

/* strspn is based on musl C-standard library implementation
   http://git.musl-libc.org/cgit/musl/tree/src/string/strcspn.c
   A custom implemenatation is necessary because C standard library impls
   have no concept of a string view and will continue searching beyond the
   end of a view until null is found. This way, string searches are
   efficient and only within the range specified. */
static size_t
sv_strcspn(const char *const str, size_t str_sz, const char *set, size_t set_sz)
{
    const char *a = str;
    size_t byteset[32 / sizeof(size_t)];
    if (!set[0])
    {
        return str_sz;
    }
    if (!set[1])
    {
        for (size_t i = 0; i < str_sz && *a && *a != *set; a++)
            ;
        return a - str;
    }
    memset(byteset, 0, sizeof byteset);
    for (size_t i = 0;
         i < set_sz && *set && BITOP(byteset, *(unsigned char *)set, |=);
         set++, ++i)
        ;
    for (size_t i = 0;
         i < str_sz && *a && !BITOP(byteset, *(unsigned char *)a, &); a++)
        ;
    return a - str;
}

/* strspn is based on musl C-standard library implementation
   https://git.musl-libc.org/cgit/musl/tree/src/string/strspn.c
   A custom implemenatation is necessary because C standard library impls
   have no concept of a string view and will continue searching beyond the
   end of a view until null is found. This way, string searches are
   efficient and only within the range specified. */
static size_t
sv_strspn(const char *const str, size_t str_sz, const char *set, size_t set_sz)
{
    const char *a = str;
    size_t byteset[32 / sizeof(size_t)] = {0};
    if (!set[0])
    {
        return str_sz;
    }
    if (!set[1])
    {
        for (size_t i = 0; i < str_sz && i < set_sz && *a == *set; a++, ++i)
            ;
        return a - str;
    }
    for (size_t i = 0;
         i < set_sz && *set && BITOP(byteset, *(unsigned char *)set, |=);
         set++, ++i)
        ;
    for (size_t i = 0;
         i < str_sz && *a && BITOP(byteset, *(unsigned char *)a, &); a++, ++i)
        ;
    return a - str;
}

/* Providing strnstrn rather than strstr at the lowest level works better
   for string views where the string may not be null terminated. There needs
   to always be the additional constraint that a search cannot exceed the
   hay length. Returns 0 based index position at which needle begins in
   hay if it can be found, otherwise the hay size is returned. */
static size_t
sv_strnstrn(const char *const hay, ssize_t hay_sz, const char *const needle,
            ssize_t needle_sz)
{
    if (!hay || !needle || !needle_sz || !*needle || needle_sz > hay_sz)
    {
        return hay_sz;
    }
    if (1 == needle_sz)
    {
        return sv_strnchr(hay, *needle, hay_sz);
    }
    if (2 == needle_sz)
    {
        return sv_twobyte_strnstrn((unsigned char *)hay, hay_sz,
                                   (unsigned char *)needle);
    }
    if (3 == needle_sz)
    {
        return sv_threebyte_strnstrn((unsigned char *)hay, hay_sz,
                                     (unsigned char *)needle);
    }
    if (4 == needle_sz)
    {
        return sv_fourbyte_strnstrn((unsigned char *)hay, hay_sz,
                                    (unsigned char *)needle);
    }
    return sv_two_way(hay, hay_sz, needle, needle_sz);
}

/* For now reverse logic for backwards searches has been separated into
   other functions. There is a possible formula to unit the reverse and
   forward logic into one set of functions, but the code is ugly. See
   the start of the reverse two-way algorithm for more. May unite if
   a clean way exists. */
static size_t
sv_rstrnstrn(const char *const hay, ssize_t hay_sz, const char *const needle,
             ssize_t needle_sz)
{
    if (!hay || !needle || !needle_sz || !*needle || needle_sz > hay_sz)
    {
        return hay_sz;
    }
    if (1 == needle_sz)
    {
        return sv_rstrnchr(hay, *needle, hay_sz);
    }
    if (2 == needle_sz)
    {
        return sv_rtwobyte_strnstrn((unsigned char *)hay, hay_sz,
                                    (unsigned char *)needle);
    }
    if (3 == needle_sz)
    {
        return sv_rthreebyte_strnstrn((unsigned char *)hay, hay_sz,
                                      (unsigned char *)needle);
    }
    if (4 == needle_sz)
    {
        return sv_rfourbyte_strnstrn((unsigned char *)hay, hay_sz,
                                     (unsigned char *)needle);
    }
    return sv_rtwo_way(hay, hay_sz, needle, needle_sz);
}

/* ==============   Post-Precomputation Two-Way Search    ==================
 */

/* Definitions for Two-Way String-Matching taken from original authors:

   CROCHEMORE M., PSV_ERRIN D., 1991, Two-way string-matching,
   Journal of the ACM 38(3):651-675.

   This algorithm is used for its simplicity and low space requirement.
   What follows is a massively simplified approximation (due to my own
   lack of knowledge) of glibc's approach with the help of the ESMAJ
   website on string matching and the original paper in ACM.

   http://igm.univ-mlv.fr/~lecroq/string/node26.html#SECTION00260

   Variable names and descriptions attempt to communicate authors' original
   meaning from the 1991 paper. */

/* Two Way string matching algorithm adapted from ESMAJ
   http://igm.univ-mlv.fr/~lecroq/string/node26.html#SECTION00260

   Assumes the needle size is shorter than hay size. Sizes are needed
   for string view operations because strings may not be null terminated
   and the string view library is most likely search a view rather than
   an entire string. Returns the position at which needle begins if found
   and the size of the hay stack if not found. */
static inline size_t
sv_two_way(const char *const hay, ssize_t hay_sz, const char *const needle,
           ssize_t needle_sz)
{
    /* ssize_t is used throughout. Is this the best choice? The two-way
       algo relies on negative numbers. This fits with size_t capabilities
       but does not feel right. Plain old signed may be better. */
    ssize_t critical_pos = 0;
    ssize_t period_dist = 0;
    /* Preprocessing to get critical position and period distance. */
    const struct sv_factorization s = sv_maximal_suffix(needle, needle_sz);
    const struct sv_factorization r = sv_maximal_suffix_rev(needle, needle_sz);
    if (s.start_critical_pos > r.start_critical_pos)
    {
        critical_pos = s.start_critical_pos;
        period_dist = s.period_dist;
    }
    else
    {
        critical_pos = r.start_critical_pos;
        period_dist = r.period_dist;
    }
    /* Determine if memoization is available due to found border/overlap. */
    if (!memcmp(needle, needle + period_dist, critical_pos + 1))
    {
        return sv_two_way_memoization((struct sv_two_way_pack){
            .hay = hay,
            .hay_sz = hay_sz,
            .needle = needle,
            .needle_sz = needle_sz,
            .period_dist = period_dist,
            .critical_pos = critical_pos,
        });
    }
    return sv_two_way_normal((struct sv_two_way_pack){
        .hay = hay,
        .hay_sz = hay_sz,
        .needle = needle,
        .needle_sz = needle_sz,
        .period_dist = period_dist,
        .critical_pos = critical_pos,
    });
}

/* Two Way string matching algorithm adapted from ESMAJ
   http://igm.univ-mlv.fr/~lecroq/string/node26.html#SECTION00260 */
static inline size_t
sv_two_way_memoization(struct sv_two_way_pack p)
{
    ssize_t l_pos = 0;
    ssize_t r_pos = 0;
    /* Eliminate worst case quadratic time complexity with memoization. */
    ssize_t memoize_shift = -1;
    while (l_pos <= p.hay_sz - p.needle_sz)
    {
        r_pos = sv_ssizet_max(p.critical_pos, memoize_shift) + 1;
        while (r_pos < p.needle_sz && p.needle[r_pos] == p.hay[r_pos + l_pos])
        {
            ++r_pos;
        }
        if (r_pos < p.needle_sz)
        {
            l_pos += (r_pos - p.critical_pos);
            memoize_shift = -1;
            continue;
        }
        /* p.r_pos >= p.needle_sz */
        r_pos = p.critical_pos;
        while (r_pos > memoize_shift && p.needle[r_pos] == p.hay[r_pos + l_pos])
        {
            --r_pos;
        }
        if (r_pos <= memoize_shift)
        {
            return l_pos;
        }
        l_pos += p.period_dist;
        /* Some prefix of needle coincides with the text. Memoize the length
           of this prefix to increase length of next shift, if possible. */
        memoize_shift = p.needle_sz - p.period_dist - 1;
    }
    return p.hay_sz;
}

/* Two Way string matching algorithm adapted from ESMAJ
   http://igm.univ-mlv.fr/~lecroq/string/node26.html#SECTION00260 */
static inline size_t
sv_two_way_normal(struct sv_two_way_pack p)
{
    p.period_dist
        = sv_ssizet_max(p.critical_pos + 1, p.needle_sz - p.critical_pos - 1)
          + 1;
    ssize_t l_pos = 0;
    ssize_t r_pos = 0;
    while (l_pos <= p.hay_sz - p.needle_sz)
    {
        r_pos = p.critical_pos + 1;
        while (r_pos < p.needle_sz && p.needle[r_pos] == p.hay[r_pos + l_pos])
        {
            ++r_pos;
        }
        if (r_pos < p.needle_sz)
        {
            l_pos += (r_pos - p.critical_pos);
            continue;
        }
        /* p.r_pos >= p.needle_sz */
        r_pos = p.critical_pos;
        while (r_pos >= 0 && p.needle[r_pos] == p.hay[r_pos + l_pos])
        {
            --r_pos;
        }
        if (r_pos < 0)
        {
            return l_pos;
        }
        l_pos += p.period_dist;
    }
    return p.hay_sz;
}

/* ================   Suffix and Critical Factorization    =================
 */

/* Computing of the maximal suffix. Adapted from ESMAJ.
   http://igm.univ-mlv.fr/~lecroq/string/node26.html#SECTION00260 */
static inline struct sv_factorization
sv_maximal_suffix(const char *const needle, ssize_t needle_sz)
{
    ssize_t suff_pos = -1;
    ssize_t period = 1;
    ssize_t last_rest = 0;
    ssize_t rest = 1;
    while (last_rest + rest < needle_sz)
    {
        switch (sv_char_cmp(needle[last_rest + rest], needle[suff_pos + rest]))
        {
        case SV_LES:
            last_rest += rest;
            rest = 1;
            period = last_rest - suff_pos;
            break;
        case SV_EQL:
            if (rest != period)
            {
                ++rest;
            }
            else
            {
                last_rest += period;
                rest = 1;
            }
            break;
        case SV_GRT:
            suff_pos = last_rest;
            last_rest = suff_pos + 1;
            rest = period = 1;
            break;
        default:
            break;
        }
    }
    return (struct sv_factorization){.start_critical_pos = suff_pos,
                                     .period_dist = period};
}

/* Computing of the maximal suffix reverse. Sometimes called tilde.
   adapted from ESMAJ
   http://igm.univ-mlv.fr/~lecroq/string/node26.html#SECTION00260 */
static inline struct sv_factorization
sv_maximal_suffix_rev(const char *const needle, ssize_t needle_sz)
{
    ssize_t suff_pos = -1;
    ssize_t period = 1;
    ssize_t last_rest = 0;
    ssize_t rest = 1;
    while (last_rest + rest < needle_sz)
    {
        switch (sv_char_cmp(needle[last_rest + rest], needle[suff_pos + rest]))
        {
        case SV_GRT:
            last_rest += rest;
            rest = 1;
            period = last_rest - suff_pos;
            break;
        case SV_EQL:
            if (rest != period)
            {
                ++rest;
            }
            else
            {
                last_rest += period;
                rest = 1;
            }
            break;
        case SV_LES:
            suff_pos = last_rest;
            last_rest = suff_pos + 1;
            rest = period = 1;
            break;
        default:
            break;
        }
    }
    return (struct sv_factorization){.start_critical_pos = suff_pos,
                                     .period_dist = period};
}

/*=======================  Right to Left Search  ===========================*/

/* Two way algorithm is easy to reverse. Instead of trying to reverse all
   logic in the factorizations and two way searches, leave the algorithms
   and calculate the values returned as offsets from the end of the string
   instead of indices starting from 0. It would be even be possible to unite
   these functions into one with the following formula
   (using the suffix calculation as an example):

ssize_t suff_pos = -1;
ssize_t period = 1;
ssize_t last_rest = 0;
ssize_t rest = 1;
ssize_t negate_sz = 0;
ssize_t negate_one = 0;
if (direction == FORWARD)
{
    negation_sz = needle_sz;
    negate_one = 1;
}
while (last_rest + rest < needle_sz)
{
    switch (sv_char_cmp(
        needle[needle_sz - (last_rest + rest) - 1 + negation_sz + negate_one],
        needle[needle_sz - (suff_pos + rest) - 1 + negation_sz + negate_one]))
    {
    ...

   That would save the code repitition across all of the following
   functions but probably would make the code even harder to read and
   maintain. These algorithms are dense enough already so I think repetion
   is fine. Leaving comment here if that changes or an even better way comes
   along. */

/* Searches a string from right to left with a two-way algorithm. Returns
   the position of the start of the strig if found and string size if not. */
static inline size_t
sv_rtwo_way(const char *const hay, ssize_t hay_sz, const char *const needle,
            ssize_t needle_sz)
{
    ssize_t critical_pos = 0;
    ssize_t period_dist = 0;
    const struct sv_factorization s = sv_rmaximal_suffix(needle, needle_sz);
    const struct sv_factorization r = sv_rmaximal_suffix_rev(needle, needle_sz);
    if (s.start_critical_pos > r.start_critical_pos)
    {
        critical_pos = s.start_critical_pos;
        period_dist = s.period_dist;
    }
    else
    {
        critical_pos = r.start_critical_pos;
        period_dist = r.period_dist;
    }
    if (!sv_rmemcmp(needle + needle_sz - 1,
                    needle + needle_sz - period_dist - 1, critical_pos + 1))
    {
        return sv_rtwo_way_memoization((struct sv_two_way_pack){
            .hay = hay,
            .hay_sz = hay_sz,
            .needle = needle,
            .needle_sz = needle_sz,
            .period_dist = period_dist,
            .critical_pos = critical_pos,
        });
    }
    return sv_rtwo_way_normal((struct sv_two_way_pack){
        .hay = hay,
        .hay_sz = hay_sz,
        .needle = needle,
        .needle_sz = needle_sz,
        .period_dist = period_dist,
        .critical_pos = critical_pos,
    });
}

static inline size_t
sv_rtwo_way_memoization(struct sv_two_way_pack p)
{
    ssize_t l_pos = 0;
    ssize_t r_pos = 0;
    ssize_t memoize_shift = -1;
    while (l_pos <= p.hay_sz - p.needle_sz)
    {
        r_pos = sv_ssizet_max(p.critical_pos, memoize_shift) + 1;
        while (r_pos < p.needle_sz
               && p.needle[p.needle_sz - r_pos - 1]
                      == p.hay[p.hay_sz - (r_pos + l_pos) - 1])
        {
            ++r_pos;
        }
        if (r_pos < p.needle_sz)
        {
            l_pos += (r_pos - p.critical_pos);
            memoize_shift = -1;
            continue;
        }
        /* p.r_pos >= p.needle_sz */
        r_pos = p.critical_pos;
        while (r_pos > memoize_shift
               && p.needle[p.needle_sz - r_pos - 1]
                      == p.hay[p.hay_sz - (r_pos + l_pos) - 1])
        {
            --r_pos;
        }
        if (r_pos <= memoize_shift)
        {
            return p.hay_sz - l_pos - p.needle_sz;
        }
        l_pos += p.period_dist;
        /* Some prefix of needle coincides with the text. Memoize the length
           of this prefix to increase length of next shift, if possible. */
        memoize_shift = p.needle_sz - p.period_dist - 1;
    }
    return p.hay_sz;
}

static inline size_t
sv_rtwo_way_normal(struct sv_two_way_pack p)
{
    p.period_dist
        = sv_ssizet_max(p.critical_pos + 1, p.needle_sz - p.critical_pos - 1)
          + 1;
    ssize_t l_pos = 0;
    ssize_t r_pos = 0;
    while (l_pos <= p.hay_sz - p.needle_sz)
    {
        r_pos = p.critical_pos + 1;
        while (r_pos < p.needle_sz
               && (p.needle[p.needle_sz - r_pos - 1]
                   == p.hay[p.hay_sz - (r_pos + l_pos) - 1]))
        {
            ++r_pos;
        }
        if (r_pos < p.needle_sz)
        {
            l_pos += (r_pos - p.critical_pos);
            continue;
        }
        /* p.r_pos >= p.needle_sz */
        r_pos = p.critical_pos;
        while (r_pos >= 0
               && p.needle[p.needle_sz - r_pos - 1]
                      == p.hay[p.hay_sz - (r_pos + l_pos) - 1])
        {
            --r_pos;
        }
        if (r_pos < 0)
        {
            return p.hay_sz - l_pos - p.needle_sz;
        }
        l_pos += p.period_dist;
    }
    return p.hay_sz;
}

static inline struct sv_factorization
sv_rmaximal_suffix(const char *const needle, ssize_t needle_sz)
{
    ssize_t suff_pos = -1;
    ssize_t period = 1;
    ssize_t last_rest = 0;
    ssize_t rest = 1;
    while (last_rest + rest < needle_sz)
    {
        switch (sv_char_cmp(needle[needle_sz - (last_rest + rest) - 1],
                            needle[needle_sz - (suff_pos + rest) - 1]))
        {
        case SV_LES:
            last_rest += rest;
            rest = 1;
            period = last_rest - suff_pos;
            break;
        case SV_EQL:
            if (rest != period)
            {
                ++rest;
            }
            else
            {
                last_rest += period;
                rest = 1;
            }
            break;
        case SV_GRT:
            suff_pos = last_rest;
            last_rest = suff_pos + 1;
            rest = period = 1;
            break;
        default:
            break;
        }
    }
    return (struct sv_factorization){.start_critical_pos = suff_pos,
                                     .period_dist = period};
}

static inline struct sv_factorization
sv_rmaximal_suffix_rev(const char *const needle, ssize_t needle_sz)
{
    ssize_t suff_pos = -1;
    ssize_t period = 1;
    ssize_t last_rest = 0;
    ssize_t rest = 1;
    while (last_rest + rest < needle_sz)
    {
        switch (sv_char_cmp(needle[needle_sz - (last_rest + rest) - 1],
                            needle[needle_sz - (suff_pos + rest) - 1]))
        {
        case SV_GRT:
            last_rest += rest;
            rest = 1;
            period = last_rest - suff_pos;
            break;
        case SV_EQL:
            if (rest != period)
            {
                ++rest;
            }
            else
            {
                last_rest += period;
                rest = 1;
            }
            break;
        case SV_LES:
            suff_pos = last_rest;
            last_rest = suff_pos + 1;
            rest = period = 1;
            break;
        default:
            break;
        }
    }
    return (struct sv_factorization){.start_critical_pos = suff_pos,
                                     .period_dist = period};
}

/* ======================   Brute Force Search    ==========================
 */

/* All brute force searches adapted from musl C library.
   http://git.musl-libc.org/cgit/musl/tree/src/string/strstr.c
   They must stop the search at hay size and therefore required slight
   modification because string views may not be null terminated. Reverse
   methods are my own additions provided to support a compliant reverse
   search for rfind which most string libraries specify must search right
   to left. Also having a reverse tokenizer is convenient and also relies
   on right to left brute force searches. */

static inline size_t
sv_strnchr(const char *s, const char c, size_t n)
{
    size_t i = 0;
    for (; n && *s != c; s++, n--, ++i)
    {}
    return i;
}

static inline size_t
sv_rstrnchr(const char *s, const char c, size_t n)
{
    const char *x = s + n - 1;
    size_t i = n;
    for (; i && *x != c; x--, --i)
    {}
    return i ? i - 1 : n;
}

static inline size_t
sv_twobyte_strnstrn(const unsigned char *h, size_t sz,
                    const unsigned char *const n)
{
    uint16_t nw = n[0] << 8 | n[1];
    uint16_t hw = h[0] << 8 | h[1];
    size_t i = 0;
    for (h++, i++; i < sz && *h && hw != nw; hw = (hw << 8) | *++h, ++i)
    {}
    return (i < sz) ? i - 1 : sz;
}

static inline size_t
sv_rtwobyte_strnstrn(const unsigned char *h, size_t sz,
                     const unsigned char *const n)
{
    h = h + sz - 2;
    uint16_t nw = n[0] << 8 | n[1];
    uint16_t hw = h[0] << 8 | h[1];
    size_t i = sz - 1;
    /* The search is right to left therefore the Most Significant Byte will
       be the leading character of the string and the previous leading
       character is shifted to the right. */
    for (; i && hw != nw; hw = (hw >> 8) | (*--h << 8), --i)
    {}
    return i ? i - 1 : sz;
}

static inline size_t
sv_threebyte_strnstrn(const unsigned char *h, size_t sz,
                      const unsigned char *const n)
{
    uint32_t nw = (uint32_t)n[0] << 24 | n[1] << 16 | n[2] << 8;
    uint32_t hw = (uint32_t)h[0] << 24 | h[1] << 16 | h[2] << 8;
    size_t i = 0;
    for (h += 2, i += 2; i < sz && *h && hw != nw; hw = (hw | *++h) << 8, ++i)
    {}
    return (i < sz) ? i - 2 : sz;
}

static inline size_t
sv_rthreebyte_strnstrn(const unsigned char *h, size_t sz,
                       const unsigned char *const n)
{
    h = h + sz - 3;
    uint32_t nw = (uint32_t)n[0] << 16 | n[1] << 8 | n[2];
    uint32_t hw = (uint32_t)h[0] << 16 | h[1] << 8 | h[2];
    size_t i = sz - 2;
    /* Align the bits with fewer left shifts such that as the parsing
       progresses right left, the leading character always takes highest
       bit position and there is no need for any masking. */
    for (; i && hw != nw; hw = (hw >> 8) | (*--h << 16), --i)
    {}
    return i ? i - 1 : sz;
}

static inline size_t
sv_fourbyte_strnstrn(const unsigned char *h, size_t sz,
                     const unsigned char *const n)
{
    uint32_t nw = (uint32_t)n[0] << 24 | n[1] << 16 | n[2] << 8 | n[3];
    uint32_t hw = (uint32_t)h[0] << 24 | h[1] << 16 | h[2] << 8 | h[3];
    size_t i = 0;
    for (h += 3, i += 3; i < sz && *h && hw != nw; hw = (hw << 8) | *++h, ++i)
    {}
    return (i < sz) ? i - 3 : sz;
}

static inline size_t
sv_rfourbyte_strnstrn(const unsigned char *h, size_t sz,
                      const unsigned char *const n)
{
    h = h + sz - 4;
    uint32_t nw = (uint32_t)n[0] << 24 | n[1] << 16 | n[2] << 8 | n[3];
    uint32_t hw = (uint32_t)h[0] << 24 | h[1] << 16 | h[2] << 8 | h[3];
    size_t i = sz - 3;
    /* Now that all four bytes of the unsigned int are used the shifting
       becomes more intuitive. The window slides left to right and the
       next leading character takes the high bit position. */
    for (; i && hw != nw; hw = (hw >> 8) | (*--h << 24), --i)
    {}
    return i ? i - 1 : sz;
}
