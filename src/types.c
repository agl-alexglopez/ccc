#include "types.h"

inline bool
ccc_entry_occupied(ccc_entry const e)
{
    return e.status & CCC_ENTRY_OCCUPIED;
}

inline bool
ccc_entry_error(ccc_entry const e)
{
    return e.status == CCC_ENTRY_ERROR;
}

inline void *
ccc_entry_unwrap(ccc_entry const e)
{
    return e.entry;
}
