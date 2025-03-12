#ifndef CCC_IMPL_TYPES_H
#define CCC_IMPL_TYPES_H

/** @cond */
#include <stddef.h>
#include <stdint.h>
/** @endcond */

/** @private */
enum ccc_entry_status_ : uint8_t
{
    CCC_VACANT = 0,
    CCC_OCCUPIED = 0x1,
    CCC_INSERT_ERROR = 0x2,
    CCC_ARG_ERROR = 0x4,
    CCC_NO_UNWRAP = 0x8,
};

/** @private */
struct ccc_ent_
{
    void *e_;
    enum ccc_entry_status_ stats_;
};

/** @private */
union ccc_entry_
{
    struct ccc_ent_ impl_;
};

/** @private */
struct ccc_handl_
{
    size_t i_;
    enum ccc_entry_status_ stats_;
};

/** @private */
union ccc_handle_
{
    struct ccc_handl_ impl_;
};

/** @private */
struct ccc_range_u_
{
    union
    {
        void *begin_;
        void *rbegin_;
    };
    union
    {
        void *end_;
        void *rend_;
    };
};

/** @private */
union ccc_range_
{
    struct ccc_range_u_ impl_;
};

/** @private */
union ccc_rrange_
{
    struct ccc_range_u_ impl_;
};

#endif /* CCC_IMPL_TYPES_H */
