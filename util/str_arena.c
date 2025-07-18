#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "str_arena.h"

static enum str_arena_result str_arena_maybe_resize(struct str_arena *a,
                                                    size_t byte_request);
static enum str_arena_result str_arena_maybe_resize_pos(struct str_arena *a,
                                                        size_t furthest_pos);

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
struct str_ofs
str_arena_alloc(struct str_arena *const a, size_t const bytes)
{
    enum str_arena_result const res = str_arena_maybe_resize(a, bytes);
    if (res)
    {
        return (struct str_ofs){.error = res};
    }
    size_t const ret = a->next_free_pos;
    a->next_free_pos += bytes;
    return (struct str_ofs){.ofs = ret, .len = bytes ? bytes - 1 : bytes};
}

enum str_arena_result
str_arena_push_back(struct str_arena *const a, struct str_ofs *const str,
                    char const c)
{
    if (!a || !str || str->error)
    {
        return STR_ARENA_ARG_ERROR;
    }
    size_t const new_pos = str->ofs + str->len + 1;
    enum str_arena_result const res = str_arena_maybe_resize_pos(a, new_pos);
    if (res)
    {
        return res;
    }
    char *const string = str_arena_at(a, str);
    *(string + str->len) = c;
    *(string + str->len + 1) = '\0';
    if (new_pos >= a->next_free_pos)
    {
        a->next_free_pos += ((new_pos + 1) - a->next_free_pos);
    }
    ++str->len;
    return STR_ARENA_OK;
}

static enum str_arena_result
str_arena_maybe_resize(struct str_arena *const a, size_t const byte_request)
{
    if (!a)
    {
        return STR_ARENA_ARG_ERROR;
    }
    return str_arena_maybe_resize_pos(a, a->next_free_pos + byte_request);
}

static enum str_arena_result
str_arena_maybe_resize_pos(struct str_arena *const a, size_t const furthest_pos)
{
    if (!a)
    {
        return STR_ARENA_ARG_ERROR;
    }
    if (furthest_pos >= a->cap)
    {
        size_t const new_cap = (furthest_pos) * 2;
        void *const moved_arena = realloc(a->arena, new_cap);
        if (!moved_arena)
        {
            return STR_ARENA_ALLOC_FAIL;
        }
        memset((char *)moved_arena + a->cap, '\0', new_cap - a->cap);
        a->arena = moved_arena;
        a->cap = new_cap;
    }
    return STR_ARENA_OK;
}

enum str_arena_result
str_arena_pop_str(struct str_arena *const a, struct str_ofs *const last_str)
{
    if (!a || !a->arena || !a->cap || !a->next_free_pos || !last_str
        || last_str->error)
    {
        return STR_ARENA_ARG_ERROR;
    }
    if (last_str->len)
    {
        memset(a->arena + last_str->ofs, '\0', last_str->len);
    }
    if (last_str->ofs + last_str->len + 1 == a->next_free_pos)
    {
        a->next_free_pos = last_str->ofs;
        last_str->len = 0;
    }
    else
    {
        *last_str = (struct str_ofs){.error = STR_ARENA_INVALID};
    }
    return STR_ARENA_OK;
}

enum str_arena_result
str_arena_clear(struct str_arena *const a)
{
    if (!a)
    {
        return STR_ARENA_ARG_ERROR;
    }
    if (a->arena)
    {
        memset(a->arena, '\0', a->cap);
    }
    a->next_free_pos = 0;
    a->cap = 0;
    return STR_ARENA_OK;
}

enum str_arena_result
str_arena_free(struct str_arena *const a)
{
    if (!a)
    {
        return STR_ARENA_ARG_ERROR;
    }
    if (a->arena)
    {
        free(a->arena);
        a->arena = NULL;
    }
    a->next_free_pos = 0;
    a->cap = 0;
    return STR_ARENA_OK;
}

char *
str_arena_at(struct str_arena const *const a, struct str_ofs const *const i)
{
    if (!a || !i || i->error || i->ofs >= a->cap)
    {
        return NULL;
    }
    return a->arena + i->ofs;
}
