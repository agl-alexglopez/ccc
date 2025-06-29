#ifndef STR_ARENA_H
#define STR_ARENA_H

#include <stddef.h>
#include <stdint.h>

typedef ptrdiff_t str_ofs;

struct str_arena
{
    char *arena;
    size_t next_free_pos;
    size_t cap;
};

/** Creates an arena of strings of the requested capacity in bytes. */
struct str_arena str_arena_create(size_t capacity);

/** Services the requested allocation of bytes. If successful the position
returned is the string offset in the arena at which contiguous bytes are
located. The bytes requested are the total allotted so do not forget to account
for the null terminator if a string is being formed in the allocation. */
str_ofs str_arena_alloc(struct str_arena *, size_t bytes);

/** Returns the last given out position to the arena. The only fine grained
freeing made possible and requires the user knows the most recently allocated
position or all strings between end and last_pos are invalidated. */
void str_arena_free_to_pos(struct str_arena *, str_ofs last_str,
                           size_t str_len);

/** Push a character back to the last string allocation. This is possible
and useful when a string may be edited depending on other factors before it is
finally recorded for later use. One would overwrite other strings if this is not
the last element. However all strings will remain null terminated. A null
terminator is added after the new character. Assumes str_len + 1 bytes for str
have been allocated already starting at str. */
bool str_arena_push_back(struct str_arena *, str_ofs str, size_t len, char);

/** Returns the NULL terminated strings starting at the string offset provided.
NULL is returned if the provided offset is out of range. */
char *str_arena_at(struct str_arena const *, str_ofs);

/** Frees the entire arena and resets all struct fields to NULL or 0. */
void str_arena_free(struct str_arena *);

#endif /* STR_ARENA_H */
