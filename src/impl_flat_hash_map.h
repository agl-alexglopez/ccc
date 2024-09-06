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

struct ccc_fhm_elem_
{
    uint64_t hash;
};

struct ccc_fhm_
{
    ccc_buffer buf;
    ccc_hash_fn *hash_fn;
    ccc_key_eq_fn *eq_fn;
    void *aux;
    size_t key_offset;
    size_t hash_elem_offset;
};

struct ccc_fhm_entry_
{
    struct ccc_fhm_ *h;
    uint64_t hash;
    ccc_entry entry;
};

#define CCC_IMPL_FHM_INIT(fhash_ptr, memory_ptr, capacity, struct_name,        \
                          key_field, fhash_elem_field, alloc_fn, hash_fn,      \
                          key_eq_fn, aux)                                      \
    ({                                                                         \
        (fhash_ptr)->impl.buf = (ccc_buffer)CCC_BUF_INIT(                      \
            (memory_ptr), struct_name, (capacity), (alloc_fn));                \
        ccc_result res_ = ccc_impl_fhm_init(                                   \
            &(fhash_ptr)->impl, offsetof(struct_name, key_field),              \
            offsetof(struct_name, fhash_elem_field), (hash_fn), (key_eq_fn),   \
            (aux));                                                            \
        res_;                                                                  \
    })

ccc_result ccc_impl_fhm_init(struct ccc_fhm_ *, size_t key_offset,
                             size_t hash_elem_offset, ccc_hash_fn *,
                             ccc_key_eq_fn *, void *aux);
ccc_entry ccc_impl_fhm_find(struct ccc_fhm_ const *, void const *key,
                            uint64_t hash);
void ccc_impl_fhm_insert(struct ccc_fhm_ *h, void const *e, uint64_t hash,
                         size_t cur_i);

struct ccc_fhm_entry_ ccc_impl_fhm_entry(struct ccc_fhm_ *h, void const *key);
struct ccc_fhm_entry_ ccc_impl_fhm_and_modify(struct ccc_fhm_entry_ e,
                                              ccc_update_fn *fn);
void const *ccc_impl_fhm_unwrap(struct ccc_fhm_entry_ *e);

struct ccc_fhm_elem_ *ccc_impl_fhm_in_slot(struct ccc_fhm_ const *h,
                                           void const *slot);
void *ccc_impl_fhm_key_in_slot(struct ccc_fhm_ const *h, void const *slot);
uint64_t *ccc_impl_fhm_hash_at(struct ccc_fhm_ const *h, size_t i);
size_t ccc_impl_fhm_distance(size_t capacity, size_t index, uint64_t hash);
ccc_result ccc_impl_fhm_maybe_resize(struct ccc_fhm_ *);
uint64_t ccc_impl_fhm_filter(struct ccc_fhm_ const *, void const *key);

#define CCC_IMPL_FHM_ENTRY(fhash_ptr, key...)                                  \
    ({                                                                         \
        __auto_type const fhm_key_ = key;                                      \
        struct ccc_fhm_entry_ fhm_ent_                                         \
            = ccc_impl_fhm_entry(&(fhash_ptr)->impl, &fhm_key_);               \
        fhm_ent_;                                                              \
    })

#define CCC_IMPL_FHM_GET(fhash_ptr, key...)                                    \
    ({                                                                         \
        __auto_type const fhm_key_ = key;                                      \
        struct ccc_fhm_entry_ fhm_get_ent_                                     \
            = ccc_impl_fhm_entry(&(fhash_ptr)->impl, &fhm_key_);               \
        void const *fhm_get_res_ = NULL;                                       \
        if (fhm_get_ent_.entry.status & CCC_FHM_ENTRY_OCCUPIED)                \
        {                                                                      \
            fhm_get_res_ = fhm_get_ent_.entry.entry;                           \
        }                                                                      \
        fhm_get_res_;                                                          \
    })

#define CCC_IMPL_FHM_GET_MUT(fhash_ptr, key...)                                \
    ({                                                                         \
        void *fhm_get_mut_res_ = (void *)CCC_IMPL_FHM_GET(fhash_ptr, key);     \
        fhm_get_mut_res_;                                                      \
    })

#define CCC_IMPL_FHM_AND_MODIFY(entry_copy, mod_fn)                            \
    ({                                                                         \
        struct ccc_fhm_entry_ fhm_mod_ent_                                     \
            = ccc_impl_fhm_and_modify((entry_copy).impl, (mod_fn));            \
        fhm_mod_ent_;                                                          \
    })

#define CCC_IMPL_FHM_AND_MODIFY_W(entry_copy, mod_fn, aux)                     \
    ({                                                                         \
        struct ccc_fhm_entry_ fhm_mod_with_ent_ = (entry_copy).impl;           \
        if (fhm_mod_with_ent_.entry.status == CCC_FHM_ENTRY_OCCUPIED)          \
        {                                                                      \
            __auto_type fhm_aux_ = aux;                                        \
            (mod_fn)((ccc_update){(void *)fhm_mod_with_ent_.entry.entry,       \
                                  &fhm_aux_});                                 \
        }                                                                      \
        fhm_mod_with_ent_;                                                     \
    })

#define CCC_IMPL_FHM_SWAPS(swap_entry, key_val_struct...)                      \
    ({                                                                         \
        size_t fhm_i_ = ccc_buf_index_of(&((swap_entry).h->buf),               \
                                         (swap_entry).entry.entry);            \
        if (*ccc_impl_fhm_hash_at((swap_entry).h, fhm_i_) == CCC_FHM_EMPTY)    \
        {                                                                      \
            *((typeof(key_val_struct) *)ccc_buf_at(&((swap_entry).h->buf),     \
                                                   fhm_i_))                    \
                = key_val_struct;                                              \
            *ccc_impl_fhm_hash_at((swap_entry).h, fhm_i_) = (swap_entry).hash; \
            ++(swap_entry).h->buf.impl.sz;                                     \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            typeof(key_val_struct) fhm_cur_slot_                               \
                = *((typeof(fhm_cur_slot_) *)ccc_buf_at(                       \
                    &((swap_entry).h->buf), fhm_i_));                          \
            *((typeof(fhm_cur_slot_) *)ccc_buf_at(&((swap_entry).h->buf),      \
                                                  fhm_i_))                     \
                = key_val_struct;                                              \
            *ccc_impl_fhm_hash_at((swap_entry).h, fhm_i_) = (swap_entry).hash; \
            fhm_i_ = (fhm_i_ + 1) % ccc_buf_capacity(&((swap_entry).h->buf));  \
            ccc_impl_fhm_insert(                                               \
                (swap_entry).h, &fhm_cur_slot_,                                \
                ccc_impl_fhm_in_slot((swap_entry).h, &fhm_cur_slot_)->hash,    \
                fhm_i_);                                                       \
        }                                                                      \
        (void *)(swap_entry).entry.entry;                                      \
    })

#define CCC_IMPL_FHM_INSERT_ENTRY(entry_copy, key_val_struct...)               \
    ({                                                                         \
        typeof(key_val_struct) *fhm_res_;                                      \
        struct ccc_fhm_entry_ fhm_ins_ent_ = (entry_copy).impl;                \
        assert(sizeof(*fhm_res_)                                               \
               == ccc_buf_elem_size(&(fhm_ins_ent_.h->buf)));                  \
        if (fhm_ins_ent_.entry.status & CCC_FHM_ENTRY_OCCUPIED)                \
        {                                                                      \
            fhm_ins_ent_.entry.status = CCC_FHM_ENTRY_OCCUPIED;                \
            *((typeof(fhm_res_))fhm_ins_ent_.entry.entry) = key_val_struct;    \
            ccc_impl_fhm_in_slot(fhm_ins_ent_.h, fhm_ins_ent_.entry.entry)     \
                ->hash                                                         \
                = fhm_ins_ent_.hash;                                           \
            fhm_res_ = (void *)fhm_ins_ent_.entry.entry;                       \
        }                                                                      \
        else if (fhm_ins_ent_.entry.status & ~CCC_FHM_ENTRY_OCCUPIED)          \
        {                                                                      \
            fhm_res_ = NULL;                                                   \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            fhm_res_ = CCC_IMPL_FHM_SWAPS(fhm_ins_ent_, key_val_struct);       \
        }                                                                      \
        fhm_res_;                                                              \
    })

#define CCC_IMPL_FHM_OR_INSERT(entry_copy, key_val_struct...)                  \
    ({                                                                         \
        typeof(key_val_struct) *res_;                                          \
        struct ccc_fhm_entry_ entry_ = (entry_copy).impl;                      \
        assert(sizeof(*res_) == ccc_buf_elem_size(&(entry_.h->buf)));          \
        if (entry_.entry.status & CCC_FHM_ENTRY_OCCUPIED)                      \
        {                                                                      \
            res_ = (void *)entry_.entry.entry;                                 \
        }                                                                      \
        else if (entry_.entry.status & ~CCC_FHM_ENTRY_VACANT)                  \
        {                                                                      \
            res_ = NULL;                                                       \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            res_ = CCC_IMPL_FHM_SWAPS(entry_, key_val_struct);                 \
        }                                                                      \
        res_;                                                                  \
    })

#endif /* CCC_IMPL_FLAT_HASH_MAP_H */
