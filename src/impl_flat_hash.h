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

ccc_entry ccc_impl_fh_find(struct ccc_impl_fhash const *, void const *key,
                           uint64_t hash);

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

#define CCC_IMPL_FH_SWAPS(swap_entry, struct_key_value_initializer...)         \
    size_t _i_                                                                 \
        = ccc_buf_index_of((swap_entry).h->buf, (swap_entry).entry.entry);     \
    typeof(struct_key_value_initializer) _cur_slot_                            \
        = *((typeof(_cur_slot_) *)ccc_buf_at((swap_entry).h->buf, _i_));       \
    *((typeof(_cur_slot_) *)ccc_buf_at((swap_entry).h->buf, _i_))              \
        = (typeof(_cur_slot_))struct_key_value_initializer;                    \
    *ccc_impl_hash_at((swap_entry).h, _i_) = (swap_entry).hash;                \
    size_t const _cap_ = ccc_buf_capacity((swap_entry).h->buf);                \
    size_t _dist_ = ccc_impl_fh_distance(_cap_, _i_, (swap_entry).hash);       \
    for (_i_ = (_i_ + 1) % _cap_;; _i_ = (_i_ + 1) % _cap_, ++_dist_)          \
    {                                                                          \
        typeof(_cur_slot_) *_slot_ = ccc_buf_at((swap_entry).h->buf, _i_);     \
        struct ccc_impl_fh_elem *_slot_hash_                                   \
            = ccc_impl_fh_in_slot((swap_entry).h, _slot_);                     \
        if (_slot_hash_->hash == EMPTY)                                        \
        {                                                                      \
            *_slot_ = (_cur_slot_);                                            \
            ++(swap_entry).h->buf->impl.sz;                                    \
            break;                                                             \
        }                                                                      \
        size_t const _slot_dist_                                               \
            = ccc_impl_fh_distance(_cap_, _i_, _slot_hash_->hash);             \
        if (_dist_ > _slot_dist_)                                              \
        {                                                                      \
            typeof(_cur_slot_) _tmp_ = *_slot_;                                \
            *_slot_ = (_cur_slot_);                                            \
            (_cur_slot_) = _tmp_;                                              \
            _dist_ = _slot_dist_;                                              \
        }                                                                      \
    }

#define CCC_IMPL_FH_INSERT_ENTRY(entry_copy, struct_key_value_initializer...)  \
    ({                                                                         \
        typeof(struct_key_value_initializer) *_res_;                           \
        struct ccc_impl_fh_entry _ins_ent_ = (entry_copy).impl;                \
        if ((_ins_ent_.entry.status                                            \
             & (CCC_ENTRY_NULL | CCC_ENTRY_SEARCH_ERROR)))                     \
        {                                                                      \
            _res_ = (void *)_ins_ent_.entry.entry;                             \
        }                                                                      \
        else if (_ins_ent_.entry.status & CCC_ENTRY_OCCUPIED)                  \
        {                                                                      \
            *((typeof(_res_))_ins_ent_.entry.entry)                            \
                = (typeof(*_res_))struct_key_value_initializer;                \
            _res_ = (void *)_ins_ent_.entry.entry;                             \
        }                                                                      \
        else if (ccc_impl_load_factor_exceeded(_ins_ent_.h))                   \
        {                                                                      \
            _res_ = NULL;                                                      \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            if (*ccc_impl_hash_at(                                             \
                    _ins_ent_.h,                                               \
                    ccc_buf_index_of(_ins_ent_.h->buf, _ins_ent_.entry.entry)) \
                == EMPTY)                                                      \
            {                                                                  \
                size_t _i_ = ccc_buf_index_of(_ins_ent_.h->buf,                \
                                              _ins_ent_.entry.entry);          \
                *((typeof(_res_))ccc_buf_at(_ins_ent_.h->buf, _i_))            \
                    = (typeof(*_res_))struct_key_value_initializer;            \
                *ccc_impl_hash_at(_ins_ent_.h, _i_) = _ins_ent_.hash;          \
                ++_ins_ent_.h->buf->impl.sz;                                   \
            }                                                                  \
            else                                                               \
            {                                                                  \
                CCC_IMPL_FH_SWAPS(_ins_ent_, (struct_key_value_initializer));  \
            }                                                                  \
            _ins_ent_.entry.entry = ccc_impl_fh_maybe_resize(                  \
                _ins_ent_.h, (void *)_ins_ent_.entry.entry);                   \
            _res_ = (void *)_ins_ent_.entry.entry;                             \
        }                                                                      \
        _res_;                                                                 \
    })

#define CCC_IMPL_FH_OR_INSERT(entry_copy, struct_key_value_initializer...)     \
    ({                                                                         \
        typeof(struct_key_value_initializer) *_res_;                           \
        struct ccc_impl_fh_entry _entry_ = (entry_copy).impl;                  \
        if (_entry_.entry.status != CCC_ENTRY_VACANT)                          \
        {                                                                      \
            _res_ = (void *)_entry_.entry.entry;                               \
        }                                                                      \
        else if (sizeof(typeof(struct_key_value_initializer))                  \
                     != ccc_buf_elem_size(_entry_.h->buf)                      \
                 || ccc_impl_load_factor_exceeded(_entry_.h))                  \
        {                                                                      \
            _res_ = NULL;                                                      \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            if (*ccc_impl_hash_at(                                             \
                    _entry_.h,                                                 \
                    ccc_buf_index_of(_entry_.h->buf, _entry_.entry.entry))     \
                == EMPTY)                                                      \
            {                                                                  \
                size_t _i_                                                     \
                    = ccc_buf_index_of(_entry_.h->buf, _entry_.entry.entry);   \
                *((typeof(struct_key_value_initializer) *)ccc_buf_at(          \
                    _entry_.h->buf, _i_))                                      \
                    = (typeof(struct_key_value_initializer))                   \
                        struct_key_value_initializer;                          \
                *ccc_impl_hash_at(_entry_.h, _i_) = _entry_.hash;              \
                ++_entry_.h->buf->impl.sz;                                     \
            }                                                                  \
            else                                                               \
            {                                                                  \
                CCC_IMPL_FH_SWAPS(_entry_, (struct_key_value_initializer));    \
            }                                                                  \
            _entry_.entry.entry = ccc_impl_fh_maybe_resize(                    \
                _entry_.h, (void *)_entry_.entry.entry);                       \
            _res_ = (void *)_entry_.entry.entry;                               \
        }                                                                      \
        _res_;                                                                 \
    })

#endif /* CCC_IMPL_FLAT_HASH_H */
