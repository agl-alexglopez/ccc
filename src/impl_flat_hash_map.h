#ifndef CCC_IMPL_FLAT_HASH_MAP_H
#define CCC_IMPL_FLAT_HASH_MAP_H

#include "buffer.h"
#include "types.h"

#include <assert.h>
#include <stdint.h>

#define CCC_FHM_EMPTY ((uint64_t)0)
#define CCC_FHM_ENTRY_VACANT ((uint8_t)0x0)
#define CCC_FHM_ENTRY_OCCUPIED ((uint8_t)0x1)
#define CCC_FHM_ENTRY_INSERT_ERROR ((uint8_t)0x2)
#define CCC_FHM_ENTRY_SEARCH_ERROR ((uint8_t)0x4)
#define CCC_FHM_ENTRY_NULL ((uint8_t)0x8)

struct ccc_impl_fhash_elem
{
    uint64_t hash;
};

struct ccc_impl_flat_hash
{
    ccc_buffer buf;
    ccc_hash_fn *hash_fn;
    ccc_key_eq_fn *eq_fn;
    void *aux;
    size_t key_offset;
    size_t hash_elem_offset;
};

struct ccc_impl_fhash_entry
{
    struct ccc_impl_flat_hash *h;
    uint64_t hash;
    ccc_entry entry;
};

#define CCC_IMPL_FHM_INIT(fhash_ptr, memory_ptr, capacity, struct_name,        \
                          key_field, fhash_elem_field, alloc_fn, hash_fn,      \
                          key_eq_fn, aux)                                      \
    ({                                                                         \
        (fhash_ptr)->impl.buf = (ccc_buffer)CCC_BUF_INIT(                      \
            (memory_ptr), struct_name, (capacity), (alloc_fn));                \
        ccc_result _res_ = ccc_impl_fhm_init(                                  \
            &(fhash_ptr)->impl, offsetof(struct_name, key_field),              \
            offsetof(struct_name, fhash_elem_field), (hash_fn), (key_eq_fn),   \
            (aux));                                                            \
        _res_;                                                                 \
    })

ccc_result ccc_impl_fhm_init(struct ccc_impl_flat_hash *, size_t key_offset,
                             size_t hash_elem_offset, ccc_hash_fn *,
                             ccc_key_eq_fn *, void *aux);
ccc_entry ccc_impl_fhm_find(struct ccc_impl_flat_hash const *, void const *key,
                            uint64_t hash);
void ccc_impl_fhm_insert(struct ccc_impl_flat_hash *h, void const *e,
                         uint64_t hash, size_t cur_i);

struct ccc_impl_fhash_entry ccc_impl_fhm_entry(struct ccc_impl_flat_hash *h,
                                               void const *key);
struct ccc_impl_fhash_entry
ccc_impl_fhm_and_modify(struct ccc_impl_fhash_entry e, ccc_update_fn *fn);
void const *ccc_impl_fhm_unwrap(struct ccc_impl_fhash_entry *e);

struct ccc_impl_fhash_elem *
ccc_impl_fhm_in_slot(struct ccc_impl_flat_hash const *h, void const *slot);
void *ccc_impl_fhm_key_in_slot(struct ccc_impl_flat_hash const *h,
                               void const *slot);
uint64_t *ccc_impl_fhm_hash_at(struct ccc_impl_flat_hash const *h, size_t i);
size_t ccc_impl_fhm_distance(size_t capacity, size_t index, uint64_t hash);
ccc_result ccc_impl_fhm_maybe_resize(struct ccc_impl_flat_hash *);
uint64_t ccc_impl_fhm_filter(struct ccc_impl_flat_hash const *,
                             void const *key);

#define CCC_IMPL_FHM_ENTRY(fhash_ptr, key...)                                  \
    ({                                                                         \
        __auto_type const _fhm_key_ = key;                                     \
        struct ccc_impl_fhash_entry _fhm_ent_                                  \
            = ccc_impl_fhm_entry(&(fhash_ptr)->impl, &_fhm_key_);              \
        _fhm_ent_;                                                             \
    })

#define CCC_IMPL_FHM_GET(fhash_ptr, key...)                                    \
    ({                                                                         \
        __auto_type const _fhm_key_ = key;                                     \
        struct ccc_impl_fhash_entry _fhm_get_ent_                              \
            = ccc_impl_fhm_entry(&(fhash_ptr)->impl, &_fhm_key_);              \
        void const *_fhm_get_res_ = NULL;                                      \
        if (_fhm_get_ent_.entry.status & CCC_FHM_ENTRY_OCCUPIED)               \
        {                                                                      \
            _fhm_get_res_ = _fhm_get_ent_.entry.entry;                         \
        }                                                                      \
        _fhm_get_res_;                                                         \
    })

#define CCC_IMPL_FHM_GET_MUT(fhash_ptr, key...)                                \
    ({                                                                         \
        void *_fhm_get_mut_res_ = (void *)CCC_IMPL_FHM_GET(fhash_ptr, key);    \
        _fhm_get_mut_res_;                                                     \
    })

#define CCC_IMPL_FHM_AND_MODIFY(entry_copy, mod_fn)                            \
    ({                                                                         \
        struct ccc_impl_fhash_entry _fhm_mod_ent_                              \
            = ccc_impl_fhm_and_modify((entry_copy).impl, (mod_fn));            \
        _fhm_mod_ent_;                                                         \
    })

#define CCC_IMPL_FHM_AND_MODIFY_W(entry_copy, mod_fn, aux)                     \
    ({                                                                         \
        struct ccc_impl_fhash_entry _fhm_mod_with_ent_ = (entry_copy).impl;    \
        if (_fhm_mod_with_ent_.entry.status == CCC_FHM_ENTRY_OCCUPIED)         \
        {                                                                      \
            __auto_type _fhm_aux_ = aux;                                       \
            (mod_fn)((ccc_update){(void *)_fhm_mod_with_ent_.entry.entry,      \
                                  &_fhm_aux_});                                \
        }                                                                      \
        _fhm_mod_with_ent_;                                                    \
    })

#define CCC_IMPL_FHM_SWAPS(swap_entry, key_val_struct...)                      \
    ({                                                                         \
        size_t _fhm_i_ = ccc_buf_index_of(&((swap_entry).h->buf),              \
                                          (swap_entry).entry.entry);           \
        if (*ccc_impl_fhm_hash_at((swap_entry).h, _fhm_i_) == CCC_FHM_EMPTY)   \
        {                                                                      \
            *((typeof(key_val_struct) *)ccc_buf_at(&((swap_entry).h->buf),     \
                                                   _fhm_i_))                   \
                = key_val_struct;                                              \
            *ccc_impl_fhm_hash_at((swap_entry).h, _fhm_i_)                     \
                = (swap_entry).hash;                                           \
            ++(swap_entry).h->buf.impl.sz;                                     \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            typeof(key_val_struct) _fhm_cur_slot_                              \
                = *((typeof(_fhm_cur_slot_) *)ccc_buf_at(                      \
                    &((swap_entry).h->buf), _fhm_i_));                         \
            *((typeof(_fhm_cur_slot_) *)ccc_buf_at(&((swap_entry).h->buf),     \
                                                   _fhm_i_))                   \
                = key_val_struct;                                              \
            *ccc_impl_fhm_hash_at((swap_entry).h, _fhm_i_)                     \
                = (swap_entry).hash;                                           \
            _fhm_i_                                                            \
                = (_fhm_i_ + 1) % ccc_buf_capacity(&((swap_entry).h->buf));    \
            ccc_impl_fhm_insert(                                               \
                (swap_entry).h, &_fhm_cur_slot_,                               \
                ccc_impl_fhm_in_slot((swap_entry).h, &_fhm_cur_slot_)->hash,   \
                _fhm_i_);                                                      \
        }                                                                      \
        (void *)(swap_entry).entry.entry;                                      \
    })

#define CCC_IMPL_FHM_INSERT_ENTRY(entry_copy, key_val_struct...)               \
    ({                                                                         \
        typeof(key_val_struct) *_fhm_res_;                                     \
        struct ccc_impl_fhash_entry _fhm_ins_ent_ = (entry_copy).impl;         \
        assert(sizeof(*_fhm_res_)                                              \
               == ccc_buf_elem_size(&(_fhm_ins_ent_.h->buf)));                 \
        if (_fhm_ins_ent_.entry.status & CCC_FHM_ENTRY_OCCUPIED)               \
        {                                                                      \
            _fhm_ins_ent_.entry.status = CCC_FHM_ENTRY_OCCUPIED;               \
            *((typeof(_fhm_res_))_fhm_ins_ent_.entry.entry) = key_val_struct;  \
            ccc_impl_fhm_in_slot(_fhm_ins_ent_.h, _fhm_ins_ent_.entry.entry)   \
                ->hash                                                         \
                = _fhm_ins_ent_.hash;                                          \
            _fhm_res_ = (void *)_fhm_ins_ent_.entry.entry;                     \
        }                                                                      \
        else if (_fhm_ins_ent_.entry.status & ~CCC_FHM_ENTRY_OCCUPIED)         \
        {                                                                      \
            _fhm_res_ = NULL;                                                  \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            _fhm_res_ = CCC_IMPL_FHM_SWAPS(_fhm_ins_ent_, key_val_struct);     \
        }                                                                      \
        _fhm_res_;                                                             \
    })

#define CCC_IMPL_FHM_OR_INSERT(entry_copy, key_val_struct...)                  \
    ({                                                                         \
        typeof(key_val_struct) *_res_;                                         \
        struct ccc_impl_fhash_entry _entry_ = (entry_copy).impl;               \
        assert(sizeof(*_res_) == ccc_buf_elem_size(&(_entry_.h->buf)));        \
        if (_entry_.entry.status & CCC_FHM_ENTRY_OCCUPIED)                     \
        {                                                                      \
            _res_ = (void *)_entry_.entry.entry;                               \
        }                                                                      \
        else if (_entry_.entry.status & ~CCC_FHM_ENTRY_VACANT)                 \
        {                                                                      \
            _res_ = NULL;                                                      \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            _res_ = CCC_IMPL_FHM_SWAPS(_entry_, key_val_struct);               \
        }                                                                      \
        _res_;                                                                 \
    })

#endif /* CCC_IMPL_FLAT_HASH_MAP_H */
