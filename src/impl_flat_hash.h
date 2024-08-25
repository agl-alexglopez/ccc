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
    ccc_buf *buf;
    ccc_hash_fn *hash_fn;
    ccc_key_cmp_fn *eq_fn;
    void *aux;
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
    ccc_entry const entry;
};

ccc_entry ccc_impl_fh_find(struct ccc_impl_fhash const *, void const *key,
                           uint64_t hash);

struct ccc_impl_fh_elem *ccc_impl_fh_in_slot(struct ccc_impl_fhash const *h,
                                             void const *slot);
size_t ccc_impl_fh_distance(size_t capacity, size_t index, uint64_t hash);
ccc_result ccc_impl_fh_maybe_resize(struct ccc_impl_fhash *);
uint64_t ccc_impl_fh_filter(struct ccc_impl_fhash const *, void const *key);

#define CCC_IMPL_FH_ENTRY(fhash_ptr, key)                                      \
    ({                                                                         \
        typeof(key) const _key_ = key;                                         \
        uint64_t const _hash_                                                  \
            = ccc_impl_fh_filter(&(fhash_ptr)->impl, &_key_);                  \
        struct ccc_impl_fh_entry _ent_ = {                                     \
            .h = &(fhash_ptr)->impl,                                           \
            .hash = _hash_,                                                    \
            .entry = ccc_impl_fh_find(&(fhash_ptr)->impl, &_key_, _hash_),     \
        };                                                                     \
        _ent_;                                                                 \
    })

#define CCC_IMPL_FH_AND_MODIFY(entry_copy, mod_fn)                             \
    ({                                                                         \
        struct ccc_impl_fh_entry _mod_ent_ = (entry_copy).impl;                \
        if (_mod_ent_.entry.status == CCC_ENTRY_OCCUPIED)                      \
        {                                                                      \
            mod_fn((ccc_update){(void *)_mod_ent_.entry.entry, NULL});         \
        }                                                                      \
        _mod_ent_;                                                             \
    })

#define CCC_IMPL_FH_AND_MODIFY_WITH(entry_copy, mod_fn, aux)                   \
    ({                                                                         \
        struct ccc_impl_fh_entry _mod_with_ent_ = (entry_copy).impl;           \
        if (_mod_with_ent_.entry.status == CCC_ENTRY_OCCUPIED)                 \
        {                                                                      \
            typeof(aux) _aux_ = aux;                                           \
            mod_fn((ccc_update){(void *)_mod_with_ent_.entry.entry, &_aux_});  \
        }                                                                      \
        _mod_with_ent_;                                                        \
    })

#define CCC_IMPL_FH_OR_INSERT_WITH(entry_copy,                                 \
                                   struct_key_value_initializer...)            \
    ({                                                                         \
        typeof(struct_key_value_initializer) *_res_;                           \
        {                                                                      \
            struct ccc_impl_fh_entry _entry_ = entry_copy.impl;                \
            if (_entry_.entry.status != CCC_ENTRY_VACANT)                      \
            {                                                                  \
                _res_ = (void *)_entry_.entry.entry;                           \
            }                                                                  \
            else if (sizeof(typeof(struct_key_value_initializer))              \
                         != ccc_buf_elem_size(_entry_.h->buf)                  \
                     || ccc_impl_fh_maybe_resize(_entry_.h) != CCC_OK)         \
            {                                                                  \
                _res_ = NULL;                                                  \
            }                                                                  \
            else                                                               \
            {                                                                  \
                size_t _i_                                                     \
                    = ccc_buf_index_of(_entry_.h->buf, _entry_.entry.entry);   \
                void *_slot_ = ccc_buf_at(_entry_.h->buf, _i_);                \
                struct ccc_impl_fh_elem *_slot_hash_                           \
                    = ccc_impl_fh_in_slot(_entry_.h, _slot_);                  \
                if (_slot_hash_->hash == EMPTY)                                \
                {                                                              \
                    *((typeof(struct_key_value_initializer) *)_slot_)          \
                        = (typeof(struct_key_value_initializer))               \
                            struct_key_value_initializer;                      \
                    _slot_hash_->hash = _entry_.hash;                          \
                    ++_entry_.h->buf->impl.sz;                                 \
                }                                                              \
                else                                                           \
                {                                                              \
                    size_t const _cap_ = ccc_buf_capacity(_entry_.h->buf);     \
                    size_t _dist_                                              \
                        = ccc_impl_fh_distance(_cap_, _i_, _entry_.hash);      \
                    typeof(struct_key_value_initializer) _cur_                 \
                        = *((typeof(struct_key_value_initializer) *)_slot_);   \
                    *((typeof(struct_key_value_initializer) *)_slot_)          \
                        = (typeof(struct_key_value_initializer))               \
                            struct_key_value_initializer;                      \
                    _slot_hash_->hash = _entry_.hash;                          \
                    for (++_i_, ++_dist_;; _i_ = (_i_ + 1) % _cap_, ++_dist_)  \
                    {                                                          \
                        _slot_ = ccc_buf_at(_entry_.h->buf, _i_);              \
                        _slot_hash_ = ccc_impl_fh_in_slot(_entry_.h, _slot_);  \
                        if (_slot_hash_->hash == EMPTY)                        \
                        {                                                      \
                            *((typeof(struct_key_value_initializer) *)_slot_)  \
                                = _cur_;                                       \
                            ++_entry_.h->buf->impl.sz;                         \
                            break;                                             \
                        }                                                      \
                        size_t const _slot_dist_ = ccc_impl_fh_distance(       \
                            _cap_, _i_, _slot_hash_->hash);                    \
                        if (_dist_ > _slot_dist_)                              \
                        {                                                      \
                            typeof(struct_key_value_initializer) _tmp_         \
                                = *((typeof(struct_key_value_initializer) *)   \
                                        _slot_);                               \
                            *((typeof(struct_key_value_initializer) *)_slot_)  \
                                = _cur_;                                       \
                            _cur_ = _tmp_;                                     \
                            _dist_ = _slot_dist_;                              \
                        }                                                      \
                    }                                                          \
                }                                                              \
                _res_ = (void *)_entry_.entry.entry;                           \
            }                                                                  \
        };                                                                     \
        _res_;                                                                 \
    })

#endif /* CCC_IMPL_FLAT_HASH_H */
