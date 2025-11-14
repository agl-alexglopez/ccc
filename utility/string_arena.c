#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "string_arena.h"

static enum String_arena_result
string_arena_maybe_resize(struct String_arena *a, size_t byte_request);
static enum String_arena_result
string_arena_maybe_resize_pos(struct String_arena *a, size_t furthest_pos);

/*=======================   Str Arena Allocator   ===========================*/

struct String_arena
string_arena_create(size_t const cap)
{
    struct String_arena a;
    a.arena = calloc(cap, sizeof(char));
    if (!a.arena)
    {
        return (struct String_arena){};
    }
    a.cap = cap;
    a.next_free_pos = 0;
    return a;
}

/* Allocates exactly bytes bytes from the arena. Do not forget the null
   terminator in requests if a string is requested. */
struct String_offset
string_arena_allocate(struct String_arena *const a, size_t const bytes)
{
    enum String_arena_result const res = string_arena_maybe_resize(a, bytes);
    if (res)
    {
        return (struct String_offset){.error = res};
    }
    size_t const ret = a->next_free_pos;
    a->next_free_pos += bytes;
    return (struct String_offset){.ofs = ret, .len = bytes ? bytes - 1 : bytes};
}

enum String_arena_result
string_arena_push_back(struct String_arena *const a,
                       struct String_offset *const str, char const c)
{
    if (!a || !str || str->error)
    {
        return STRING_ARENA_ARGUMENT_ERROR;
    }
    size_t const new_pos = str->ofs + str->len + 1;
    enum String_arena_result const res
        = string_arena_maybe_resize_pos(a, new_pos);
    if (res)
    {
        return res;
    }
    char *const string = string_arena_at(a, str);
    *(string + str->len) = c;
    *(string + str->len + 1) = '\0';
    if (new_pos >= a->next_free_pos)
    {
        a->next_free_pos += ((new_pos + 1) - a->next_free_pos);
    }
    ++str->len;
    return STRING_ARENA_OK;
}

static enum String_arena_result
string_arena_maybe_resize(struct String_arena *const a,
                          size_t const byte_request)
{
    if (!a)
    {
        return STRING_ARENA_ARGUMENT_ERROR;
    }
    return string_arena_maybe_resize_pos(a, a->next_free_pos + byte_request);
}

static enum String_arena_result
string_arena_maybe_resize_pos(struct String_arena *const a,
                              size_t const furthest_pos)
{
    if (!a)
    {
        return STRING_ARENA_ARGUMENT_ERROR;
    }
    if (furthest_pos >= a->cap)
    {
        size_t const new_cap = (furthest_pos) * 2;
        void *const moved_arena = realloc(a->arena, new_cap);
        if (!moved_arena)
        {
            return STRING_ARENA_ALLOCATION_FAIL;
        }
        memset((char *)moved_arena + a->cap, '\0', new_cap - a->cap);
        a->arena = moved_arena;
        a->cap = new_cap;
    }
    return STRING_ARENA_OK;
}

enum String_arena_result
string_arena_pop_str(struct String_arena *const a,
                     struct String_offset *const last_str)
{
    if (!a || !a->arena || !a->cap || !a->next_free_pos || !last_str
        || last_str->error)
    {
        return STRING_ARENA_ARGUMENT_ERROR;
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
        *last_str = (struct String_offset){.error = STRING_ARENA_INVALID};
    }
    return STRING_ARENA_OK;
}

enum String_arena_result
string_arena_clear(struct String_arena *const a)
{
    if (!a)
    {
        return STRING_ARENA_ARGUMENT_ERROR;
    }
    if (a->arena)
    {
        memset(a->arena, '\0', a->cap);
    }
    a->next_free_pos = 0;
    a->cap = 0;
    return STRING_ARENA_OK;
}

enum String_arena_result
string_arena_free(struct String_arena *const a)
{
    if (!a)
    {
        return STRING_ARENA_ARGUMENT_ERROR;
    }
    if (a->arena)
    {
        free(a->arena);
        a->arena = NULL;
    }
    a->next_free_pos = 0;
    a->cap = 0;
    return STRING_ARENA_OK;
}

char *
string_arena_at(struct String_arena const *const a,
                struct String_offset const *const i)
{
    if (!a || !i || i->error || i->ofs >= a->cap)
    {
        return NULL;
    }
    return a->arena + i->ofs;
}
