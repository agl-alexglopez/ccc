#ifndef STRING_ARENA_H
#define STRING_ARENA_H

#include <stddef.h>
#include <stdint.h>

/** Used to indicate the status of various API requests. Any non-zero status
means an error has occurred, simplifying error checking. */
enum String_arena_result : uint8_t
{
    STRING_ARENA_OK = 0,
    STRING_ARENA_ARG_ERROR,
    STRING_ARENA_ALLOC_FAIL,
    STRING_ARENA_INVALID,
};

/** A str_ofs is the position in the arena at which an allocation request
exists. Upon allocation the error field will be set to STRING_ARENA_OK if the
allocation succeeded. If any error occurred the status will be non-zero and
the offset index should not be accessed. */
struct String_offset
{
    /** The status of the offset, STRING_ARENA_OK or any non-zero error. */
    enum String_arena_result error;
    /** The starting index of the string in the arena. */
    size_t ofs;
    /** The length of this string in the arena. */
    size_t len;
};

/** A contiguous arena of characters. Each str_ofs returned from the API
represents a null terminated allocation of characters. The arena offers the
ability to allocate new strings and edit the most recent allocation. This means
the arena is basically a bump allocator that allows some editing of the most
recent bump allocation in order to enable minimal dynamic string operations.

Assume that any request to allocate a string or push back characters may result
in a resizing operation. This is why indices are returned not pointers. Pointers
into the arena should only be accessed for reading or writing, not saved. */
struct String_arena
{
    /** The underlying base of allocation all strings are offset from. */
    char *arena;
    /** The front of the free list or next position in arena available. */
    size_t next_free_pos;
    /** The total bytes in this arena. */
    size_t cap;
};

/** Creates an arena of strings of the requested starting capacity in bytes. */
struct String_arena string_arena_create(size_t capacity);

/** Services the requested allocation of bytes. If successful the position
returned is the string offset in the arena at which contiguous bytes are
located. The bytes requested are the total allotted so do not forget to account
for the null terminator. */
struct String_offset string_arena_allocate(struct String_arena *, size_t bytes);

/** Pop the last string from the arena and resets the next free position to the
starting position of last_str. If the last_str argument is the last string
previously allocated from the arena its length is reset to zero but its position
remains valid. If this function is called on a string that is not the most
recently allocated that region is simply zeroed out and the arena's next free
position remains unchanged; in this case the last_str argument is then made
invalid and will not be usable in further API functions. */
enum String_arena_result string_arena_pop_str(struct String_arena *,
                                              struct String_offset *last_str);

/** Push a character back to the last string allocation. This is possible
and useful when a string may be edited depending on other factors before it is
finally recorded for later use. One would overwrite other strings if this is not
the last element. This will result in unexpected string reads for the
overwritten string but all strings will remain NULL terminated. */
enum String_arena_result
string_arena_push_back(struct String_arena *, struct String_offset *str, char);

/** Returns the NULL terminated string starting at the string offset provided.
NULL is returned if the input offset has an error or is out of range. */
char *string_arena_at(struct String_arena const *,
                      struct String_offset const *);

/** Maintain the arena allocation but clear all strings from the arena such
the next request receives the first free position. */
enum String_arena_result string_arena_clear(struct String_arena *);

/** Frees the entire arena and resets all struct fields to NULL or 0. */
enum String_arena_result string_arena_free(struct String_arena *);

#endif /* STRING_ARENA_H */
