#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "str_arena.h"

static int str_arena_maybe_resize(struct str_arena *a, size_t byte_request);
static int str_arena_maybe_resize_pos(struct str_arena *a, size_t furthest_pos);

/*=======================   Str Arena Allocator   ===========================*/

struct str_arena
str_arena_create(size_t const cap)
{
    struct str_arena a;
    a.arena = calloc(cap, sizeof(char));
    if (!a.arena)
    {
        return (struct str_arena){};
    }
    a.cap = cap;
    a.next_free_pos = 0;
    return a;
}

/* Allocates exactly bytes bytes from the arena. Do not forget the null
   terminator in requests if a string is requested. */
str_ofs
str_arena_alloc(struct str_arena *const a, size_t const bytes)
{
    if (str_arena_maybe_resize(a, bytes) == -1)
    {
        return -1;
    }
    size_t const ret = a->next_free_pos;
    a->next_free_pos += bytes;
    return (ptrdiff_t)ret;
}

bool
str_arena_push_back(struct str_arena *const a, str_ofs const str,
                    size_t const str_len, char const c)
{
    size_t const new_pos = str + str_len + 1;
    if (str_arena_maybe_resize_pos(a, new_pos) == -1)
    {
        return false;
    }
    char *const string = str_arena_at(a, str);
    *(string + str_len) = c;
    *(string + str_len + 1) = '\0';
    if (new_pos >= a->next_free_pos)
    {
        a->next_free_pos += ((new_pos + 1) - a->next_free_pos);
    }
    return true;
}

static int
str_arena_maybe_resize(struct str_arena *const a, size_t const byte_request)
{
    if (!a)
    {
        return -1;
    }
    return str_arena_maybe_resize_pos(a, a->next_free_pos + byte_request);
}

static int
str_arena_maybe_resize_pos(struct str_arena *const a, size_t const furthest_pos)
{
    if (!a)
    {
        return -1;
    }
    if (furthest_pos >= a->cap)
    {
        size_t const new_cap = (furthest_pos) * 2;
        void *const moved_arena = realloc(a->arena, new_cap);
        if (!moved_arena)
        {
            return -1;
        }
        memset((char *)moved_arena + a->cap, '\0', new_cap - a->cap);
        a->arena = moved_arena;
        a->cap = new_cap;
    }
    return 0;
}

void
str_arena_free_to_pos(struct str_arena *const a, str_ofs const last_str,
                      size_t const str_len)
{
    if (!a || !a->arena || !a->cap || !a->next_free_pos)
    {
        return;
    }
    if (str_len)
    {
        memset(a->arena + last_str, '\0', str_len);
    }
    a->next_free_pos = last_str;
}

void
str_arena_free(struct str_arena *const a)
{
    if (!a)
    {
        return;
    }
    if (a->arena)
    {
        free(a->arena);
        a->arena = NULL;
    }
    a->next_free_pos = 0;
    a->cap = 0;
}

char *
str_arena_at(struct str_arena const *const a, str_ofs const i)
{
    if (!a || (size_t)i >= a->cap)
    {
        return NULL;
    }
    return a->arena + i;
}
