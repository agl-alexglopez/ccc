#ifndef CCC_IMPL_FLAT_HASH_H
#define CCC_IMPL_FLAT_HASH_H

#include "buf.h"
#include "types.h"

#include <stdint.h>

#define EMPTY ((uint64_t)-1)

struct ccc_impl_fhash_elem
{
    uint64_t hash;
};

struct ccc_impl_flat_hash
{
    ccc_buf *buf;
    ccc_hash_fn *hash_fn;
    ccc_key_cmp_fn *eq_fn;
    void *aux;
    size_t hash_elem_offset;
};

struct ccc_impl_fhash_entry
{
    struct ccc_impl_flat_hash *const h;
    uint64_t const hash;
    ccc_entry const entry;
};

ccc_entry ccc_impl_fhash_find(struct ccc_impl_flat_hash const *,
                              void const *key, uint64_t hash);

struct ccc_impl_fhash_elem *
ccc_impl_fhash_in_slot(struct ccc_impl_flat_hash const *h, void const *slot);
size_t ccc_impl_fhash_distance(size_t capacity, size_t index, uint64_t hash);
ccc_result ccc_impl_fhash_maybe_resize(struct ccc_impl_flat_hash *);
uint64_t ccc_impl_fhash_filter(struct ccc_impl_flat_hash const *,
                               void const *key);

#define CCC_IMPL_FHASH_ENTRY(fhash_ptr, key)                                   \
    {                                                                          \
        .impl = ({                                                             \
            typeof(key) const _key_ = key;                                     \
            uint64_t const _hash_                                              \
                = ccc_impl_fhash_filter(&(fhash_ptr)->impl, &_key_);           \
            struct ccc_impl_fhash_entry _ent_ = {                              \
                .h = &(fhash_ptr)->impl,                                       \
                .hash = _hash_,                                                \
                .entry                                                         \
                = ccc_impl_fhash_find(&(fhash_ptr)->impl, &_key_, _hash_),     \
            };                                                                 \
            _ent_;                                                             \
        })                                                                     \
    }

#define CCC_IMPL_FHASH_OR_INSERT_WITH(entry_copy,                              \
                                      struct_key_value_initializer...)         \
    ({                                                                         \
        void *_res_;                                                           \
        {                                                                      \
            struct ccc_impl_fhash_entry _entry_ = entry_copy.impl;             \
            if (_entry_.entry.occupied)                                        \
            {                                                                  \
                _res_ = _entry_.entry.entry;                                   \
            }                                                                  \
            else if (sizeof(typeof(struct_key_value_initializer))              \
                         != ccc_buf_elem_size(_entry_.h->buf)                  \
                     || !_entry_.entry.entry                                   \
                     || ccc_impl_fhash_maybe_resize(_entry_.h) != CCC_OK)      \
            {                                                                  \
                _res_ = NULL;                                                  \
            }                                                                  \
            else                                                               \
            {                                                                  \
                size_t _i_                                                     \
                    = ccc_buf_index_of(_entry_.h->buf, _entry_.entry.entry);   \
                size_t _dist_ = 0;                                             \
                size_t const _cap_ = ccc_buf_capacity(_entry_.h->buf);         \
                typeof(struct_key_value_initializer) _cur_;                    \
                typeof(struct_key_value_initializer) _tmp_;                    \
                bool _initialized_ = false;                                    \
                for (;; _i_ = (_i_ + 1) % _cap_, ++_dist_)                     \
                {                                                              \
                    void *const _slot_ = ccc_buf_at(_entry_.h->buf, _i_);      \
                    struct ccc_impl_fhash_elem *_slot_hash_                    \
                        = ccc_impl_fhash_in_slot(_entry_.h, _slot_);           \
                    if (_slot_hash_->hash == EMPTY)                            \
                    {                                                          \
                        if (_initialized_)                                     \
                        {                                                      \
                            *((typeof(struct_key_value_initializer) *)_slot_)  \
                                = _cur_;                                       \
                        }                                                      \
                        else                                                   \
                        {                                                      \
                            *((typeof(struct_key_value_initializer) *)_slot_)  \
                                = (typeof(struct_key_value_initializer))       \
                                    struct_key_value_initializer;              \
                            _slot_hash_->hash = _entry_.hash;                  \
                        }                                                      \
                        ++_entry_.h->buf->impl.sz;                             \
                        break;                                                 \
                    }                                                          \
                    if (_dist_ > ccc_impl_fhash_distance(_cap_, _i_,           \
                                                         _slot_hash_->hash))   \
                    {                                                          \
                        if (_initialized_)                                     \
                        {                                                      \
                            _tmp_ = *((typeof(struct_key_value_initializer) *) \
                                          _slot_);                             \
                            *((typeof(struct_key_value_initializer) *)_slot_)  \
                                = _cur_;                                       \
                            _cur_ = _tmp_;                                     \
                        }                                                      \
                        else                                                   \
                        {                                                      \
                            _cur_ = *((typeof(struct_key_value_initializer) *) \
                                          _slot_);                             \
                            *((typeof(struct_key_value_initializer) *)_slot_)  \
                                = (typeof(struct_key_value_initializer))       \
                                    struct_key_value_initializer;              \
                            _initialized_ = true;                              \
                        }                                                      \
                    }                                                          \
                }                                                              \
                _res_ = _entry_.entry.entry;                                   \
            }                                                                  \
        };                                                                     \
        _res_;                                                                 \
    })

#endif /* CCC_IMPL_FLAT_HASH_H */
