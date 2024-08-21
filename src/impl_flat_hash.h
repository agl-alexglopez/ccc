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
    ccc_eq_fn *eq_fn;
    void *aux;
    size_t hash_elem_offset;
};

struct ccc_impl_fhash_entry
{
    struct ccc_impl_flat_hash *h;
    ccc_entry entry;
};

ccc_entry ccc_impl_fhash_find(struct ccc_impl_flat_hash const *, void const *,
                              uint64_t hash);

struct ccc_impl_fhash_elem *
ccc_impl_fhash_in_slot(struct ccc_impl_flat_hash const *h, void const *slot);
size_t ccc_impl_fhash_distance(size_t capacity, size_t index, uint64_t hash);

#define CCC_IMPL_FHASH_ENTRY(fhash_ptr, struct_name,                           \
                             struct_key_initializer...)                        \
    {                                                                          \
        .impl = ({                                                             \
            struct ccc_impl_fhash_entry _ent_;                                 \
            {                                                                  \
                if (sizeof(struct_name)                                        \
                    != ccc_buf_elem_size((fhash_ptr)->impl.buf))               \
                {                                                              \
                    _ent_ = (struct ccc_impl_fhash_entry){0};                  \
                }                                                              \
                else                                                           \
                {                                                              \
                    struct_name _stack_struct_ = struct_key_initializer;       \
                    uint64_t _hash_                                            \
                        = (fhash_ptr)->impl.hash_fn(&_stack_struct_);          \
                    _ent_ = (struct ccc_impl_fhash_entry){                     \
                        .h = &(fhash_ptr)->impl,                               \
                        .entry = ccc_impl_fhash_find(&(fhash_ptr)->impl,       \
                                                     &_stack_struct_, _hash_), \
                    };                                                         \
                }                                                              \
            };                                                                 \
            _ent_;                                                             \
        })                                                                     \
    }

#define CCC_IMPL_FHASH_OR_INSERT(entry_copy, struct_name,                      \
                                 struct_key_value_initializer...)              \
    ({                                                                         \
        ccc_result _res_;                                                      \
        {                                                                      \
            struct ccc_impl_fhash_entry _entry_ = entry_copy.impl;             \
            if (sizeof(struct_name) != ccc_buf_elem_size(_entry_.h->buf)       \
                || !_entry_.entry.entry)                                       \
            {                                                                  \
                _res_ = CCC_INPUT_ERR;                                         \
            }                                                                  \
            else if (_entry_.entry.found)                                      \
            {                                                                  \
                _res_ = CCC_NOP;                                               \
            }                                                                  \
            else                                                               \
            {                                                                  \
                size_t _i_                                                     \
                    = ccc_buf_index_of(_entry_.h->buf, _entry_.entry.entry);   \
                size_t _dist_ = 0;                                             \
                size_t const _cap_ = ccc_buf_capacity(_entry_.h->buf);         \
                struct_name _cur_;                                             \
                struct_name _tmp_;                                             \
                bool _initialized_ = false;                                    \
                for (;; _i_ = (_i_ + 1) % _cap_, ++_dist_)                     \
                {                                                              \
                    void *const _slot_ = ccc_buf_at(_entry_.h->buf, _i_);      \
                    struct ccc_impl_fhash_elem const *_slot_hash_              \
                        = ccc_impl_fhash_in_slot(_entry_.h, _slot_);           \
                    if (_slot_hash_->hash == EMPTY)                            \
                    {                                                          \
                        if (_initialized_)                                     \
                        {                                                      \
                            *((struct_name *)_slot_) = _cur_;                  \
                        }                                                      \
                        else                                                   \
                        {                                                      \
                            *((struct_name *)_slot_)                           \
                                = (struct_name)struct_key_value_initializer;   \
                        }                                                      \
                        ++_entry_.h->buf->impl.sz;                             \
                        break;                                                 \
                    }                                                          \
                    if (_dist_ > ccc_impl_fhash_distance(_cap_, _i_,           \
                                                         _slot_hash_->hash))   \
                    {                                                          \
                        if (_initialized_)                                     \
                        {                                                      \
                            _tmp_ = *((struct_name *)_slot_);                  \
                            *((struct_name *)_slot_) = _cur_;                  \
                            _cur_ = _tmp_;                                     \
                        }                                                      \
                        else                                                   \
                        {                                                      \
                            _cur_ = *((struct_name *)_slot_);                  \
                            *((struct_name *)_slot_)                           \
                                = (struct_name)struct_key_value_initializer;   \
                            _initialized_ = true;                              \
                        }                                                      \
                    }                                                          \
                }                                                              \
                _res_ = CCC_OK;                                                \
            }                                                                  \
        };                                                                     \
        _res_;                                                                 \
    })

#endif /* CCC_IMPL_FLAT_HASH_H */
