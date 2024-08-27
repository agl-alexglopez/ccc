#ifndef CCC_IMPL_FLAT_HASH_H
#define CCC_IMPL_FLAT_HASH_H

#include "buf.h"
#include "types.h"

#include <stdint.h>

#define EMPTY ((uint64_t)-1)

struct ccc_impl_fh_elem
{
    uint64_t hash;
};

struct ccc_impl_fhash
{
    ccc_buf buf;
    ccc_hash_fn *hash_fn;
    ccc_key_cmp_fn *eq_fn;
    void *aux;
    size_t key_offset;
    size_t hash_elem_offset;
};

/* The underlying memory for a hash table is mutable. This is what makes it
   possible to add and remove elements. However, the user should not poke
   around in a entry. They should only use the provided functions. The
   functions can then cast away const when needed, which only occurs for
   the memory pointed to by the entry. This is well-defined because the memory
   to which an entry points is always modifiable. */
struct ccc_impl_fh_entry
{
    struct ccc_impl_fhash *const h;
    uint64_t const hash;
    ccc_entry entry;
};

#define CCC_IMPL_FH_INIT(fhash_ptr, memory_ptr, capacity, struct_name,         \
                         key_field, fhash_elem_field, realloc_fn, hash_fn,     \
                         key_cmp_fn, aux)                                      \
    ({                                                                         \
        (fhash_ptr)->impl.buf = (ccc_buf)CCC_BUF_INIT(                         \
            (memory_ptr), struct_name, (capacity), (realloc_fn));              \
        ccc_result _res_ = ccc_impl_fh_init(                                   \
            &(fhash_ptr)->impl, offsetof(struct_name, key_field),              \
            offsetof(struct_name, fhash_elem_field), (hash_fn), (key_cmp_fn),  \
            (aux));                                                            \
        _res_;                                                                 \
    })

ccc_result ccc_impl_fh_init(struct ccc_impl_fhash *, size_t key_offset,
                            size_t hash_elem_offset, ccc_hash_fn *,
                            ccc_key_cmp_fn *, void *aux);
ccc_entry ccc_impl_fh_find(struct ccc_impl_fhash const *, void const *key,
                           uint64_t hash);
void ccc_impl_fh_insert(struct ccc_impl_fhash *h, void const *e, uint64_t hash,
                        size_t cur_i);

struct ccc_impl_fh_elem *ccc_impl_fh_in_slot(struct ccc_impl_fhash const *h,
                                             void const *slot);
void *ccc_impl_key_in_slot(struct ccc_impl_fhash const *h, void const *slot);
uint64_t *ccc_impl_hash_at(struct ccc_impl_fhash const *h, size_t i);

size_t ccc_impl_fh_distance(size_t capacity, size_t index, uint64_t hash);
void *ccc_impl_fh_maybe_resize(struct ccc_impl_fhash *,
                               void *track_after_resize);
uint64_t ccc_impl_fh_filter(struct ccc_impl_fhash const *, void const *key);
bool ccc_impl_load_factor_exceeded(struct ccc_impl_fhash const *);

#define CCC_IMPL_FH_ENTRY(fhash_ptr, key)                                      \
    ({                                                                         \
        __auto_type const _key_ = (key);                                       \
        uint64_t const _hash_                                                  \
            = ccc_impl_fh_filter(&(fhash_ptr)->impl, &_key_);                  \
        struct ccc_impl_fh_entry _ent_ = {                                     \
            .h = &(fhash_ptr)->impl,                                           \
            .hash = _hash_,                                                    \
            .entry = ccc_impl_fh_find(&(fhash_ptr)->impl, &_key_, _hash_),     \
        };                                                                     \
        _ent_;                                                                 \
    })

#define CCC_IMPL_FH_GET(entry_copy)                                            \
    ({                                                                         \
        struct ccc_impl_fh_entry _get_ent_ = (entry_copy).impl;                \
        void *_ret_ = NULL;                                                    \
        if (_get_ent_.entry.status == CCC_ENTRY_OCCUPIED)                      \
        {                                                                      \
            _ret_ = (void *)_get_ent_.entry.entry;                             \
        }                                                                      \
        _ret_;                                                                 \
    })

#define CCC_IMPL_FH_AND_MODIFY(entry_copy, mod_fn)                             \
    ({                                                                         \
        struct ccc_impl_fh_entry _mod_ent_ = (entry_copy).impl;                \
        if (_mod_ent_.entry.status == CCC_ENTRY_OCCUPIED)                      \
        {                                                                      \
            (mod_fn)((ccc_update){(void *)_mod_ent_.entry.entry, NULL});       \
        }                                                                      \
        _mod_ent_;                                                             \
    })

#define CCC_IMPL_FH_AND_MODIFY_WITH(entry_copy, mod_fn, aux)                   \
    ({                                                                         \
        struct ccc_impl_fh_entry _mod_with_ent_ = (entry_copy).impl;           \
        if (_mod_with_ent_.entry.status == CCC_ENTRY_OCCUPIED)                 \
        {                                                                      \
            typeof(aux) _aux_ = aux;                                           \
            (mod_fn)(                                                          \
                (ccc_update){(void *)_mod_with_ent_.entry.entry, &_aux_});     \
        }                                                                      \
        _mod_with_ent_;                                                        \
    })

#define CCC_IMPL_FH_SWAPS(swap_entry, key_val_struct...)                       \
    size_t _i_                                                                 \
        = ccc_buf_index_of(&((swap_entry).h->buf), (swap_entry).entry.entry);  \
    if (*ccc_impl_hash_at((swap_entry).h, _i_) == EMPTY)                       \
    {                                                                          \
        *((typeof(key_val_struct) *)ccc_buf_at(&((swap_entry).h->buf), _i_))   \
            = (typeof(key_val_struct))key_val_struct;                          \
        *ccc_impl_hash_at((swap_entry).h, _i_) = (swap_entry).hash;            \
        ++(swap_entry).h->buf.impl.sz;                                         \
    }                                                                          \
    else                                                                       \
    {                                                                          \
        typeof(key_val_struct) _cur_slot_ = *(                                 \
            (typeof(_cur_slot_) *)ccc_buf_at(&((swap_entry).h->buf), _i_));    \
        *((typeof(_cur_slot_) *)ccc_buf_at(&((swap_entry).h->buf), _i_))       \
            = (typeof(_cur_slot_))key_val_struct;                              \
        *ccc_impl_hash_at((swap_entry).h, _i_) = (swap_entry).hash;            \
        _i_ = (_i_ + 1) % ccc_buf_capacity(&((swap_entry).h->buf));            \
        ccc_impl_fh_insert(                                                    \
            (swap_entry).h, &_cur_slot_,                                       \
            ccc_impl_fh_in_slot((swap_entry).h, &_cur_slot_)->hash, _i_);      \
    }                                                                          \
    (swap_entry).entry.entry = ccc_impl_fh_maybe_resize(                       \
        (swap_entry).h, (void *)(swap_entry).entry.entry);                     \
    _res_ = (void *)(swap_entry).entry.entry;

#define CCC_IMPL_FH_INSERT_ENTRY(entry_copy, key_val_struct...)                \
    ({                                                                         \
        typeof(key_val_struct) *_res_;                                         \
        struct ccc_impl_fh_entry _ins_ent_ = (entry_copy).impl;                \
        if ((_ins_ent_.entry.status                                            \
             & (CCC_ENTRY_NULL | CCC_ENTRY_SEARCH_ERROR)))                     \
        {                                                                      \
            _res_ = (void *)_ins_ent_.entry.entry;                             \
        }                                                                      \
        else if (_ins_ent_.entry.status & CCC_ENTRY_OCCUPIED)                  \
        {                                                                      \
            *((typeof(_res_))_ins_ent_.entry.entry)                            \
                = (typeof(*_res_))key_val_struct;                              \
            _res_ = (void *)_ins_ent_.entry.entry;                             \
        }                                                                      \
        else if (ccc_impl_load_factor_exceeded(_ins_ent_.h))                   \
        {                                                                      \
            _res_ = NULL;                                                      \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            CCC_IMPL_FH_SWAPS(_ins_ent_, (key_val_struct));                    \
        }                                                                      \
        _res_;                                                                 \
    })

#define CCC_IMPL_FH_OR_INSERT(entry_copy, key_val_struct...)                   \
    ({                                                                         \
        typeof(key_val_struct) *_res_;                                         \
        struct ccc_impl_fh_entry _entry_ = (entry_copy).impl;                  \
        if (_entry_.entry.status != CCC_ENTRY_VACANT)                          \
        {                                                                      \
            _res_ = (void *)_entry_.entry.entry;                               \
        }                                                                      \
        else if (sizeof(typeof(key_val_struct))                                \
                     != ccc_buf_elem_size(&(_entry_.h->buf))                   \
                 || ccc_impl_load_factor_exceeded(_entry_.h))                  \
        {                                                                      \
            _res_ = NULL;                                                      \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            CCC_IMPL_FH_SWAPS(_entry_, (key_val_struct));                      \
        }                                                                      \
        _res_;                                                                 \
    })

#endif /* CCC_IMPL_FLAT_HASH_H */
