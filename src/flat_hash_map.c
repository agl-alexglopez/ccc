#include <assert.h>
#include <limits.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "flat_hash_map.h"
#include "impl/impl_flat_hash_map.h"
#include "impl/impl_types.h"
#include "types.h"

#if defined(__GNUC__) || defined(__clang__)
#    define unlikely(expr) __builtin_expect(!!(expr), 0)
#    define likely(expr) __builtin_expect(!!(expr), 1)
#else
#    define unlikely(expr) expr
#    define likely(expr) expr
#endif

/** @private The following test should ensure some safety in assumptions we make
when the user defines a fixed size map type. */
struct internal_data_and_tag_layout_test
{
    struct
    {
        size_t s;
        uint8_t u;
    } type_with_padding_data[3];
    ccc_fhm_tag tag[2];
};
/* The type must actually get an allocation on the given platform to validate
some memory layout assumptions. This should be sufficient and the assumptions
will hold if the user happens to allocate a fixed size map on the stack. */
static struct internal_data_and_tag_layout_test test_layout_allocation;
/* The size of the valuable user data and tags should be what we expect. No
hidden padding should violate our mental model of the bytes occupied by
contiguous user data and metadata tags. We don't care about padding after the
tag array which may very well exist. */
static_assert((char *)&test_layout_allocation.tag[2]
                  - (char *)&test_layout_allocation.type_with_padding_data[0]
              == ((sizeof(test_layout_allocation.type_with_padding_data[0]) * 3)
                  + sizeof(ccc_fhm_tag) * (2)));
/* We must ensure that the tags array starts at the exact next byte boundary
after the user data type. This is required due to how we access tags and user
data via index. Data is accessed with pointer subtraction from the tags[0]
array. The tags array 0th element is the shared base for both arrays.

Data indexes backwards and tags indexes forward. Here we index too far for our
type with padding to ensure the next assertion will hold when we index from
the shared tags base address and subtract to find user data. */
static_assert((char *)&test_layout_allocation.type_with_padding_data[3]
              == (char *)&test_layout_allocation.tag);
/* Here is an example of how we access user data slots. We take whatever the
index is of the tag element and add one. Then we subtract the bytes needed to
access this user data slot. We add one because our shared base index is at the
next byte boundary AFTER the final user data element. We also always have
an extra allocation of the user type at the start of the data array for swapping
purposes. */
static_assert(
    (char *)(test_layout_allocation.tag
             - ((1 + 1)
                * sizeof(test_layout_allocation.type_with_padding_data[0])))
    == (char *)&test_layout_allocation.type_with_padding_data[1]);

/* Can we vectorize instructions? Also it is possible to specify we want a
portable implementation. Consider exposing to user in header docs. */
#if defined(__x86_64) && defined(__SSE2__) && !defined(CCC_FHM_PORTABLE)
#    include <immintrin.h>

typedef struct
{
    __m128i v;
} group;

typedef struct
{
    uint16_t v;
} index_mask;

enum : typeof((index_mask){}.v)
{
    INDEX_MASK_MSB = 0x8000,
};

#else

typedef struct
{
    uint64_t v;
} group;

typedef struct
{
    typeof((group){}.v) v;
} index_mask;

enum : typeof((index_mask){}.v)
{
    INDEX_MASK_MSB = 0x8000000000000000,
};

enum : typeof((group){}.v)
{
    GROUP_LSB = 0x101010101010101,
    GROUP_DELETED = 0x8080808080808080,
};

#endif /* defined(__x86_64) && defined(__SSE2__) &&                            \
          !defined(CCC_FHM_GENERIC)*/

enum : size_t
{
    GROUP_BYTES = sizeof(group),
    GROUP_BITS = GROUP_BYTES * CHAR_BIT,
    SIZE_T_MSB = 0x1000000000000000,
};

enum : uint8_t
{
    TAG_MSB = 0x80,
    TAG_LSB = 0x1,
    LOWER_7_BITS_MASK = 0x7F,
    TAG_BITS = sizeof(ccc_fhm_tag) * CHAR_BIT,
};

struct triangular_seq
{
    size_t i;
    size_t stride;
};

/*===========================   Prototypes   ================================*/

static void swap(char tmp[const], void *a, void *b, size_t ab_size);
static struct ccc_fhash_entry_ container_entry(struct ccc_fhmap_ *h,
                                               void const *key);
static struct ccc_handl_ handle(struct ccc_fhmap_ *h, void const *key,
                                uint64_t hash);
static struct ccc_handl_ find_key_or_slot(struct ccc_fhmap_ const *h,
                                          void const *key, uint64_t hash);
static ccc_ucount find_key(struct ccc_fhmap_ const *h, void const *key,
                           uint64_t hash);
static size_t find_known_insert_slot(struct ccc_fhmap_ const *h, uint64_t hash);
static ccc_result maybe_rehash(struct ccc_fhmap_ *h, size_t to_add);
static void insert(struct ccc_fhmap_ *h, void const *key_val_type,
                   ccc_fhm_tag m, size_t i);
static void erase(struct ccc_fhmap_ *h, size_t i);
static void rehash_in_place(struct ccc_fhmap_ *h);
static ccc_result rehash_resize(struct ccc_fhmap_ *h, size_t to_add);
static void *key_at(struct ccc_fhmap_ const *h, size_t i);
static void *data_at(struct ccc_fhmap_ const *h, size_t i);
static void *key_in_slot(struct ccc_fhmap_ const *h, void const *slot);
static void *swap_slot(struct ccc_fhmap_ const *h);
static ccc_ucount data_i(struct ccc_fhmap_ const *h, void const *data_slot);
static size_t mask_to_total_bytes(size_t elem_size, size_t mask);
static size_t mask_to_tag_bytes(size_t mask);
static size_t mask_to_data_bytes(size_t elem_size, size_t mask);
static void set_insert(struct ccc_fhmap_ *h, ccc_fhm_tag m, size_t i);
static size_t mask_to_load_factor_cap(size_t mask);
static size_t max(size_t a, size_t b);

static void set_tag(struct ccc_fhmap_ *h, ccc_fhm_tag m, size_t i);
static ccc_tribool is_index_on(index_mask m);
static size_t lowest_on_index(index_mask m);
static size_t trailing_zeros(index_mask m);
static size_t leading_zeros(index_mask m);
static size_t next_index(index_mask *m);
static ccc_tribool is_full(ccc_fhm_tag m);
static ccc_tribool is_constant(ccc_fhm_tag m);
static ccc_fhm_tag to_tag(uint64_t hash);
static group load_group(ccc_fhm_tag const *src);
static void store_group(ccc_fhm_tag *dst, group src);
static index_mask match_tag(group g, ccc_fhm_tag m);
static index_mask match_empty(group g);
static index_mask match_empty_or_deleted(group g);
static index_mask match_full(group g);
static group make_constants_empty_and_full_deleted(group g);
static unsigned countr_0(index_mask m);
static unsigned countl_0(index_mask m);
static unsigned countl_0_size_t(size_t n);
static size_t next_power_of_two(size_t n);
static ccc_tribool is_power_of_two(size_t n);

/*===========================    Interface   ================================*/

ccc_tribool
ccc_fhm_is_empty(ccc_flat_hash_map const *const h)
{
    if (unlikely(!h))
    {
        return CCC_TRIBOOL_ERROR;
    }
    return !h->sz_;
}

ccc_ucount
ccc_fhm_size(ccc_flat_hash_map const *const h)
{
    if (!h)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = h->sz_};
}

ccc_ucount
ccc_fhm_capacity(ccc_flat_hash_map const *const h)
{
    if (!h)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = h->mask_ + 1};
}

ccc_tribool
ccc_fhm_contains(ccc_flat_hash_map const *const h, void const *const key)
{
    if (unlikely(!h || !key))
    {
        return CCC_TRIBOOL_ERROR;
    }
    if (unlikely(!h->init_ || !h->sz_))
    {
        return CCC_FALSE;
    }
    return CCC_RESULT_OK
           == find_key(
                  h, key,
                  h->hash_fn_((ccc_user_key){.user_key = key, .aux = h->aux_}))
                  .error;
}

void *
ccc_fhm_get_key_val(ccc_flat_hash_map const *const h, void const *const key)
{
    if (unlikely(!h || !key || !h->init_ || !h->sz_))
    {
        return NULL;
    }
    ccc_ucount const i = find_key(
        h, key, h->hash_fn_((ccc_user_key){.user_key = key, .aux = h->aux_}));
    if (i.error)
    {
        return NULL;
    }
    return data_at(h, i.count);
}

ccc_fhmap_entry
ccc_fhm_entry(ccc_flat_hash_map *const h, void const *const key)
{
    if (unlikely(!h || !key))
    {
        return (ccc_fhmap_entry){{.handle_ = {.stats_ = CCC_ENTRY_ARG_ERROR}}};
    }
    return (ccc_fhmap_entry){container_entry(h, key)};
}

void *
ccc_fhm_or_insert(ccc_fhmap_entry const *const e, void const *key_val_type)
{

    if (unlikely(!e || !key_val_type))
    {
        return NULL;
    }
    if (e->impl_.handle_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        return data_at(e->impl_.h_, e->impl_.handle_.i_);
    }
    if (e->impl_.handle_.stats_ & CCC_ENTRY_INSERT_ERROR)
    {
        return NULL;
    }
    insert(e->impl_.h_, key_val_type, e->impl_.tag_, e->impl_.handle_.i_);
    return data_at(e->impl_.h_, e->impl_.handle_.i_);
}

void *
ccc_fhm_insert_entry(ccc_fhmap_entry const *const e, void const *key_val_type)
{

    if (unlikely(!e || !key_val_type))
    {
        return NULL;
    }
    if (e->impl_.handle_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        void *const slot = data_at(e->impl_.h_, e->impl_.handle_.i_);
        (void)memcpy(slot, key_val_type, e->impl_.h_->elem_sz_);
        return slot;
    }
    if (e->impl_.handle_.stats_ & CCC_ENTRY_INSERT_ERROR)
    {
        return NULL;
    }
    insert(e->impl_.h_, key_val_type, e->impl_.tag_, e->impl_.handle_.i_);
    return data_at(e->impl_.h_, e->impl_.handle_.i_);
}

ccc_entry
ccc_fhm_remove_entry(ccc_fhmap_entry const *const e)
{
    if (unlikely(!e))
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_ARG_ERROR}};
    }
    if (e->impl_.handle_.stats_ != CCC_ENTRY_OCCUPIED)
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_VACANT}};
    }
    erase(e->impl_.h_, e->impl_.handle_.i_);
    return (ccc_entry){{.stats_ = CCC_ENTRY_OCCUPIED}};
}

ccc_fhmap_entry *
ccc_fhm_and_modify(ccc_fhmap_entry *const e, ccc_update_fn *const fn)
{
    if (e && fn && (e->impl_.handle_.stats_ & CCC_ENTRY_OCCUPIED) != 0)
    {
        fn((ccc_user_type){data_at(e->impl_.h_, e->impl_.handle_.i_), NULL});
    }
    return e;
}

ccc_fhmap_entry *
ccc_fhm_and_modify_aux(ccc_fhmap_entry *const e, ccc_update_fn *const fn,
                       void *const aux)
{
    if (e && fn && (e->impl_.handle_.stats_ & CCC_ENTRY_OCCUPIED) != 0)
    {
        fn((ccc_user_type){data_at(e->impl_.h_, e->impl_.handle_.i_), aux});
    }
    return e;
}

ccc_entry
ccc_fhm_swap_entry(ccc_flat_hash_map *const h, void *const key_val_type_output)
{
    if (unlikely(!h || !key_val_type_output))
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_ARG_ERROR}};
    }
    void *const key = key_in_slot(h, key_val_type_output);
    struct ccc_fhash_entry_ ent = container_entry(h, key);
    if (ent.handle_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        swap(swap_slot(h), data_at(h, ent.handle_.i_), key_val_type_output,
             h->elem_sz_);
        return (ccc_entry){
            {.e_ = key_val_type_output, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    if (ent.handle_.stats_ & CCC_ENTRY_INSERT_ERROR)
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_INSERT_ERROR}};
    }
    insert(ent.h_, key_val_type_output, ent.tag_, ent.handle_.i_);
    return (ccc_entry){
        {.e_ = data_at(h, ent.handle_.i_), .stats_ = CCC_ENTRY_VACANT}};
}

ccc_entry
ccc_fhm_try_insert(ccc_flat_hash_map *const h, void *const key_val_type)
{
    if (unlikely(!h || !key_val_type))
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_ARG_ERROR}};
    }
    void *const key = key_in_slot(h, key_val_type);
    struct ccc_fhash_entry_ ent = container_entry(h, key);
    if (ent.handle_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        return (ccc_entry){
            {.e_ = data_at(h, ent.handle_.i_), .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    if (ent.handle_.stats_ & CCC_ENTRY_INSERT_ERROR)
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_INSERT_ERROR}};
    }
    insert(ent.h_, key_val_type, ent.tag_, ent.handle_.i_);
    return (ccc_entry){
        {.e_ = data_at(h, ent.handle_.i_), .stats_ = CCC_ENTRY_VACANT}};
}

ccc_entry
ccc_fhm_insert_or_assign(ccc_flat_hash_map *const h, void *const key_val_type)
{
    if (unlikely(!h || !key_val_type))
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_ARG_ERROR}};
    }
    void *const key = key_in_slot(h, key_val_type);
    struct ccc_fhash_entry_ ent = container_entry(h, key);
    if (ent.handle_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        (void)memcpy(data_at(h, ent.handle_.i_), key_val_type, h->elem_sz_);
        return (ccc_entry){
            {.e_ = data_at(h, ent.handle_.i_), .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    if (ent.handle_.stats_ & CCC_ENTRY_INSERT_ERROR)
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_INSERT_ERROR}};
    }
    insert(ent.h_, key_val_type, ent.tag_, ent.handle_.i_);
    return (ccc_entry){
        {.e_ = data_at(h, ent.handle_.i_), .stats_ = CCC_ENTRY_VACANT}};
}

ccc_entry
ccc_fhm_remove(ccc_flat_hash_map *const h, void *const key_val_type_output)
{
    if (unlikely(!h || !key_val_type_output))
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_ARG_ERROR}};
    }
    if (unlikely(!h->init_ || !h->sz_))
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_VACANT}};
    }
    void *const key = key_in_slot(h, key_val_type_output);
    ccc_ucount const index = find_key(
        h, key, h->hash_fn_((ccc_user_key){.user_key = key, .aux = h->aux_}));
    if (index.error)
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_VACANT}};
    }
    (void)memcpy(key_val_type_output, data_at(h, index.count), h->elem_sz_);
    erase(h, index.count);
    return (ccc_entry){
        {.e_ = key_val_type_output, .stats_ = CCC_ENTRY_OCCUPIED}};
}

void *
ccc_fhm_begin(ccc_flat_hash_map const *const h)
{
    if (unlikely(!h || !h->mask_ || !h->init_ || !h->mask_ || !h->sz_))
    {
        return NULL;
    }
    for (size_t i = 0; i < (h->mask_ + 1); ++i)
    {
        if (is_full(h->tag_[i]))
        {
            return data_at(h, i);
        }
    }
    return NULL;
}

void *
ccc_fhm_next(ccc_flat_hash_map const *const h, void *const key_val_type_iter)
{
    if (unlikely(!h || !key_val_type_iter || !h->mask_ || !h->init_ || !h->sz_))
    {
        return NULL;
    }
    ccc_ucount i = data_i(h, key_val_type_iter);
    if (i.error)
    {
        return NULL;
    }
    for (; i.count < (h->mask_ + 1); ++i.count)
    {
        if (is_full(h->tag_[i.count]))
        {
            return data_at(h, i.count);
        }
    }
    return NULL;
}

void *
ccc_fhm_end(ccc_flat_hash_map const *const)
{
    return NULL;
}

void *
ccc_fhm_unwrap(ccc_fhmap_entry const *const e)
{
    if (unlikely(!e) || !(e->impl_.handle_.stats_ & CCC_ENTRY_OCCUPIED))
    {
        return NULL;
    }
    return data_at(e->impl_.h_, e->impl_.handle_.i_);
}

ccc_result
ccc_fhm_clear(ccc_flat_hash_map *const h, ccc_destructor_fn *const fn)
{
    if (unlikely(!h))
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!fn)
    {
        if (unlikely(!h->init_ || !h->mask_ || !h->tag_))
        {
            return CCC_RESULT_OK;
        }
        (void)memset(h->tag_, CCC_FHM_EMPTY, mask_to_tag_bytes(h->mask_));
        h->avail_ = mask_to_load_factor_cap(h->mask_);
        h->sz_ = 0;
        return CCC_RESULT_OK;
    }
    for (size_t i = 0; i < (h->mask_ + 1); ++i)
    {
        if (is_full(h->tag_[i]))
        {
            fn((ccc_user_type){.user_type = data_at(h, i), .aux = h->aux_});
        }
    }
    (void)memset(h->tag_, CCC_FHM_EMPTY, mask_to_tag_bytes(h->mask_));
    h->avail_ = mask_to_load_factor_cap(h->mask_);
    h->sz_ = 0;
    return CCC_RESULT_OK;
}

ccc_result
ccc_fhm_clear_and_free(ccc_flat_hash_map *const h, ccc_destructor_fn *const fn)
{
    if (unlikely(!h || !h->data_))
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!h->alloc_fn_)
    {
        (void)ccc_fhm_clear(h, fn);
        return CCC_RESULT_NO_ALLOC;
    }
    if (fn)
    {
        for (size_t i = 0; i < (h->mask_ + 1); ++i)
        {
            if (is_full(h->tag_[i]))
            {
                fn((ccc_user_type){.user_type = data_at(h, i), .aux = h->aux_});
            }
        }
    }
    h->avail_ = 0;
    h->mask_ = 0;
    h->init_ = CCC_FALSE;
    h->sz_ = 0;
    h->tag_ = NULL;
    h->data_ = h->alloc_fn_(h->data_, 0, h->aux_);
    return CCC_RESULT_OK;
}

ccc_tribool
ccc_fhm_occupied(ccc_fhmap_entry const *const e)
{
    if (unlikely(!e))
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->impl_.handle_.stats_ & CCC_ENTRY_OCCUPIED) != 0;
}

ccc_tribool
ccc_fhm_insert_error(ccc_fhmap_entry const *const e)
{
    if (unlikely(!e))
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->impl_.handle_.stats_ & CCC_ENTRY_INSERT_ERROR) != 0;
}

ccc_handle_status
ccc_fhm_handle_status(ccc_fhmap_entry const *const e)
{
    if (unlikely(!e))
    {
        return CCC_ENTRY_ARG_ERROR;
    }
    return e->impl_.handle_.stats_;
}

void *
ccc_fhm_data(ccc_flat_hash_map const *h)
{
    return h ? h->data_ : NULL;
}

ccc_result
ccc_fhm_copy(ccc_flat_hash_map *const dst, ccc_flat_hash_map const *const src,
             ccc_alloc_fn *const fn)
{
    if (!dst || !src || src == dst
        || (src->mask_ && !is_power_of_two(src->mask_ + 1)))
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (dst->mask_ < src->mask_ && !fn)
    {
        return CCC_RESULT_NO_ALLOC;
    }
    /* The destination could be messed up in a variety of ways that make it
       incompatible with src. Overwrite everything and save what we need from
       dst for a smooth copy over. */
    void *const dst_data = dst->data_;
    void *const dst_tag = dst->tag_;
    size_t const dst_mask = dst->mask_;
    size_t const dst_avail = dst->avail_;
    ccc_tribool const dst_init = dst->init_;
    ccc_alloc_fn *const dst_alloc = dst->alloc_fn_;
    *dst = *src;
    dst->data_ = dst_data;
    dst->tag_ = dst_tag;
    dst->mask_ = dst_mask;
    dst->avail_ = dst_avail;
    dst->init_ = dst_init;
    dst->alloc_fn_ = dst_alloc;
    if (!src->mask_ || !src->init_)
    {
        return CCC_RESULT_OK;
    }
    size_t const src_bytes = mask_to_total_bytes(src->elem_sz_, src->mask_);
    if (dst->mask_ < src->mask_)
    {
        void *const new_mem = dst->alloc_fn_(dst->data_, src_bytes, dst->aux_);
        if (!new_mem)
        {
            return CCC_RESULT_MEM_ERROR;
        }
        dst->data_ = new_mem;
        /* Static assertions at top of file ensure this is correct. */
        dst->tag_
            = (ccc_fhm_tag *)((char *)new_mem
                              + mask_to_data_bytes(src->elem_sz_, src->mask_));
        dst->mask_ = src->mask_;
    }
    if (!dst->data_ || !src->data_)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    (void)memset(dst->tag_, CCC_FHM_EMPTY, mask_to_tag_bytes(dst->mask_));
    dst->avail_ = mask_to_load_factor_cap(dst->mask_);
    dst->sz_ = 0;
    dst->init_ = CCC_TRUE;
    for (size_t i = 0; i < (src->mask_ + 1); ++i)
    {
        if (is_full(src->tag_[i]))
        {
            uint64_t const hash = src->hash_fn_(
                (ccc_user_key){.user_key = key_at(src, i), .aux = src->aux_});
            size_t const new_i = find_known_insert_slot(dst, hash);
            set_tag(dst, to_tag(hash), new_i);
            (void)memcpy(data_at(dst, new_i), data_at(src, i), dst->elem_sz_);
        }
    }
    dst->avail_ -= src->sz_;
    dst->sz_ = src->sz_;
    return CCC_RESULT_OK;
}

ccc_tribool
ccc_fhm_validate(ccc_flat_hash_map const *const h)
{
    if (!h)
    {
        return CCC_TRIBOOL_ERROR;
    }
    /* We initialized the metadata array of 0 capacity table? Not possible. */
    if (h->init_ && !h->mask_)
    {
        return CCC_FALSE;
    }
    /* No point checking invariants when lazy init hasn't happened yet. */
    if (!h->init_ || !h->mask_)
    {
        return CCC_TRUE;
    }
    /* We are initialized, these need to point to the array positions. */
    if (!h->tag_ || !h->data_)
    {
        return CCC_FALSE;
    }
    /* Exceeded allowable load factor when determining available and size. */
    /* The replica group should be in sync. */
    for (size_t original = 0, clone = (h->mask_ + 1);
         original < CCC_FHM_GROUP_SIZE; ++original, ++clone)
    {
        if (h->tag_[original].v != h->tag_[clone].v)
        {
            return CCC_FALSE;
        }
    }
    size_t occupied = 0;
    size_t avail = 0;
    size_t deleted = 0;
    for (size_t i = 0; i < (h->mask_ + 1); ++i)
    {
        ccc_fhm_tag const t = h->tag_[i];
        /* If we are a special constant there are only two possible values. */
        if (is_constant(t) && t.v != CCC_FHM_DELETED && t.v != CCC_FHM_EMPTY)
        {
            return CCC_FALSE;
        }
        if (t.v == CCC_FHM_EMPTY)
        {
            ++avail;
        }
        else if (t.v == CCC_FHM_DELETED)
        {
            ++deleted;
        }
        else if (is_full(t))
        {
            if (to_tag(h->hash_fn_((ccc_user_key){.user_key = data_at(h, i),
                                                  .aux = h->aux_}))
                    .v
                != t.v)
            {
                return CCC_FALSE;
            }
            ++occupied;
        }
    }
    if (occupied != h->sz_ || occupied + avail + deleted != h->mask_ + 1
        || mask_to_load_factor_cap(occupied + avail + deleted) - occupied
                   - deleted
               != h->avail_)
    {
        return CCC_FALSE;
    }
    return CCC_TRUE;
}

/*======================     Private Interface      =========================*/

struct ccc_fhash_entry_
ccc_impl_fhm_entry(struct ccc_fhmap_ *const h, void const *const key)
{
    return container_entry(h, key);
}

void
ccc_impl_fhm_insert(struct ccc_fhmap_ *h, void const *key_val_type,
                    ccc_fhm_tag m, size_t i)
{
    insert(h, key_val_type, m, i);
}

void
ccc_impl_fhm_erase(struct ccc_fhmap_ *h, size_t i)
{
    erase(h, i);
}

void *
ccc_impl_fhm_data_at(struct ccc_fhmap_ const *const h, size_t const i)
{
    return data_at(h, i);
}

void *
ccc_impl_fhm_key_at(struct ccc_fhmap_ const *const h, size_t const i)
{
    return key_at(h, i);
}

void
ccc_impl_fhm_set_insert(struct ccc_fhash_entry_ const *const e)
{
    return set_insert(e->h_, e->tag_, e->handle_.i_);
}

/*=========================   Static Internals   ============================*/

/** Returns the container entry prepared for further insertion, removal, or
searched queries. This entry gives a reference to the associated map and any
metadata and location info necessary for future actions. If this entry was
obtained in hopes of insertions but insertion will cause an error. A status
flag in the handle field will indicate the error. */
static struct ccc_fhash_entry_
container_entry(struct ccc_fhmap_ *const h, void const *const key)
{
    uint64_t const hash = h->hash_fn_((ccc_user_key){key, h->aux_});
    return (struct ccc_fhash_entry_){
        .h_ = (struct ccc_fhmap_ *)h,
        .tag_ = to_tag(hash),
        .handle_ = handle(h, key, hash),
    };
}

static struct ccc_handl_
handle(struct ccc_fhmap_ *const h, void const *const key, uint64_t const hash)
{
    ccc_entry_status upcoming_insertion_error = 0;
    switch (maybe_rehash(h, 1))
    {
    case CCC_RESULT_OK:
        break;
    case CCC_RESULT_ARG_ERROR:
        return (struct ccc_handl_){.stats_ = CCC_ENTRY_ARG_ERROR};
        break;
    default:
        upcoming_insertion_error = CCC_ENTRY_INSERT_ERROR;
        break;
    };
    struct ccc_handl_ res = find_key_or_slot(h, key, hash);
    res.stats_ |= upcoming_insertion_error;
    return res;
}

static void
insert(struct ccc_fhmap_ *const h, void const *const key_val_type,
       ccc_fhm_tag const m, size_t const i)
{
    set_insert(h, m, i);
    (void)memcpy(data_at(h, i), key_val_type, h->elem_sz_);
}

static inline void
set_insert(struct ccc_fhmap_ *const h, ccc_fhm_tag const m, size_t const i)
{
    assert(i <= h->mask_);
    assert((m.v & TAG_MSB) == 0);
    h->avail_ -= (h->tag_[i].v == CCC_FHM_EMPTY);
    ++h->sz_;
    set_tag(h, m, i);
}

static void
erase(struct ccc_fhmap_ *const h, size_t const i)
{
    assert(i <= h->mask_);
    size_t const i_before = (i - CCC_FHM_GROUP_SIZE) & h->mask_;
    index_mask const i_before_empty
        = match_empty(load_group(&h->tag_[i_before]));
    index_mask const i_empty = match_empty(load_group(&h->tag_[i]));
    ccc_fhm_tag const m = leading_zeros(i_before_empty) + leading_zeros(i_empty)
                                  >= CCC_FHM_GROUP_SIZE
                              ? (ccc_fhm_tag){CCC_FHM_DELETED}
                              : (ccc_fhm_tag){CCC_FHM_EMPTY};
    h->avail_ += (m.v == CCC_FHM_EMPTY);
    --h->sz_;
    set_tag(h, m, i);
}

/** Finds the specified hash or first available slot where the hash could be
inserted. If the element does not exist and a non-occupied slot is returned
that slot will have been the first empty or deleted slot encountered in the
probe sequence. This function assumes an empty slot exists in the table. */
static struct ccc_handl_
find_key_or_slot(struct ccc_fhmap_ const *const h, void const *const key,
                 uint64_t const hash)
{
    ccc_fhm_tag const tag = to_tag(hash);
    size_t const mask = h->mask_;
    struct triangular_seq seq = {.i = hash & mask, .stride = 0};
    ccc_ucount empty_or_deleted = {.error = CCC_RESULT_FAIL};
    do
    {
        group const g = load_group(&h->tag_[seq.i]);
        index_mask m = match_tag(g, tag);
        for (size_t i_match = lowest_on_index(m); i_match != CCC_FHM_GROUP_SIZE;
             i_match = next_index(&m))
        {
            i_match = (seq.i + i_match) & mask;
            if (h->eq_fn_((ccc_key_cmp){.key_lhs = key,
                                        .user_type_rhs = data_at(h, i_match),
                                        .aux = h->aux_}))
            {
                return (struct ccc_handl_){.i_ = i_match,
                                           .stats_ = CCC_ENTRY_OCCUPIED};
            }
        }
        if (empty_or_deleted.error)
        {
            size_t const i_take = lowest_on_index(match_empty_or_deleted(g));
            if (i_take != CCC_FHM_GROUP_SIZE)
            {
                empty_or_deleted.count = (seq.i + i_take) & mask;
                empty_or_deleted.error = CCC_RESULT_OK;
            }
        }
        if (is_index_on(match_empty(g)))
        {
            return (struct ccc_handl_){.i_ = empty_or_deleted.count,
                                       .stats_ = CCC_ENTRY_VACANT};
        }
        seq.stride += CCC_FHM_GROUP_SIZE;
        seq.i += seq.stride;
        seq.i &= mask;
    } while (1);
}

/** Finds key or quits when first empty slot is encountered after a group fails
to match. This function is better when a simple lookup is needed as a few
branches and loads of groups are omitted compared to the search with intention
to insert or remove. A successful search returns the index with an OK status
while a failed search indicates a failure error. */
static ccc_ucount
find_key(struct ccc_fhmap_ const *const h, void const *const key,
         uint64_t const hash)
{
    ccc_fhm_tag const tag = to_tag(hash);
    size_t const mask = h->mask_;
    struct triangular_seq seq = {.i = hash & mask, .stride = 0};
    do
    {
        group const g = load_group(&h->tag_[seq.i]);
        index_mask m = match_tag(g, tag);
        for (size_t i_match = lowest_on_index(m); i_match != CCC_FHM_GROUP_SIZE;
             i_match = next_index(&m))
        {
            i_match = (seq.i + i_match) & mask;
            if (likely(h->eq_fn_(
                    (ccc_key_cmp){.key_lhs = key,
                                  .user_type_rhs = data_at(h, i_match),
                                  .aux = h->aux_})))
            {
                return (ccc_ucount){.count = i_match};
            }
        }
        if (is_index_on(match_empty(g)))
        {
            return (ccc_ucount){.error = CCC_RESULT_FAIL};
        }
        seq.stride += CCC_FHM_GROUP_SIZE;
        seq.i += seq.stride;
        seq.i &= mask;
    } while (1);
}

/** Finds an insert slot or loops forever. The caller of this function must know
that there is an available empty or deleted slot in the table. */
static size_t
find_known_insert_slot(struct ccc_fhmap_ const *const h, uint64_t const hash)
{
    size_t const mask = h->mask_;
    struct triangular_seq seq = {.i = hash & mask, .stride = 0};
    do
    {
        size_t const i = lowest_on_index(
            match_empty_or_deleted(load_group(&h->tag_[seq.i])));
        if (likely(i != CCC_FHM_GROUP_SIZE))
        {
            return (seq.i + i) & mask;
        }
        seq.stride += CCC_FHM_GROUP_SIZE;
        seq.i += seq.stride;
        seq.i &= mask;
    } while (1);
}

static ccc_result
maybe_rehash(struct ccc_fhmap_ *const h, size_t const to_add)
{
    if (unlikely(!h->mask_ && !h->alloc_fn_))
    {
        return CCC_RESULT_NO_ALLOC;
    }
    size_t required_total_cap = ((h->sz_ + to_add) * 8) / 7;
    if (!is_power_of_two(required_total_cap))
    {
        required_total_cap = next_power_of_two(required_total_cap);
    }
    if (unlikely(!h->init_))
    {
        if (h->mask_)
        {
            if (!h->tag_ || h->mask_ + 1 < required_total_cap)
            {
                return CCC_RESULT_MEM_ERROR;
            }
            if (h->mask_ + 1 < CCC_FHM_GROUP_SIZE
                || !is_power_of_two(h->mask_ + 1))
            {
                return CCC_RESULT_ARG_ERROR;
            }
            (void)memset(h->tag_, CCC_FHM_EMPTY, mask_to_tag_bytes(h->mask_));
        }
        h->init_ = CCC_TRUE;
    }
    if (unlikely(!h->mask_))
    {
        required_total_cap = max(required_total_cap, CCC_FHM_GROUP_SIZE);
        size_t const total_bytes
            = mask_to_total_bytes(h->elem_sz_, required_total_cap - 1);
        void *const buf = h->alloc_fn_(NULL, total_bytes, h->aux_);
        if (!buf)
        {
            return CCC_RESULT_MEM_ERROR;
        }
        h->mask_ = required_total_cap - 1;
        h->data_ = buf;
        h->avail_ = mask_to_load_factor_cap(h->mask_);
        /* Static assertions at top of file ensure this is correct. */
        h->tag_ = (ccc_fhm_tag *)((char *)buf
                                  + mask_to_data_bytes(h->elem_sz_, h->mask_));
        (void)memset(h->tag_, CCC_FHM_EMPTY, mask_to_tag_bytes(h->mask_));
    }
    if (likely(h->avail_))
    {
        return CCC_RESULT_OK;
    }
    size_t const current_total_cap = h->mask_ + 1;
    if (h->alloc_fn_ && (h->sz_ + to_add) > current_total_cap / 2)
    {
        assert(h->alloc_fn_);
        return rehash_resize(h, to_add);
    }
    if (h->sz_ == mask_to_load_factor_cap(h->mask_))
    {
        return CCC_RESULT_NO_ALLOC;
    }
    rehash_in_place(h);
    return CCC_RESULT_OK;
}

static void
rehash_in_place(struct ccc_fhmap_ *const h)
{
    assert((h->mask_ + 1) % CCC_FHM_GROUP_SIZE == 0);
    size_t const mask = h->mask_;
    size_t const allowed_cap = mask_to_load_factor_cap(mask);
    for (size_t i = 0; i < mask + 1; i += CCC_FHM_GROUP_SIZE)
    {
        store_group(&h->tag_[i], make_constants_empty_and_full_deleted(
                                     load_group(&h->tag_[i])));
    }
    (void)memcpy(h->tag_ + (mask + 1), h->tag_, CCC_FHM_GROUP_SIZE);
    h->avail_ = allowed_cap - h->sz_;
    for (size_t i = 0; i < mask + 1; ++i)
    {
        if (h->tag_[i].v != CCC_FHM_DELETED)
        {
            continue;
        }
        do
        {
            uint64_t const hash = h->hash_fn_(
                (ccc_user_key){.user_key = key_at(h, i), .aux = h->aux_});
            ccc_fhm_tag const hash_tag = to_tag(hash);
            size_t const new_slot = find_known_insert_slot(h, hash);
            size_t const hash_pos = (hash & mask);
            /* No point relocating when scanning works by groups and the probe
               sequence would place us in this group anyway. This is a valid
               position because imagine if such a placement occurred during
               the normal insertion algorithm due to some combination of
               occupied, deleted, and empty slots. */
            if ((((i - hash_pos) & mask) / CCC_FHM_GROUP_SIZE)
                == (((new_slot - hash_pos) & mask) / CCC_FHM_GROUP_SIZE))
            {
                set_tag(h, hash_tag, i);
                break; /* continues outer loop */
            }
            ccc_fhm_tag const prev = h->tag_[new_slot];
            set_tag(h, hash_tag, new_slot);
            if (prev.v == CCC_FHM_EMPTY)
            {
                set_tag(h, (ccc_fhm_tag){CCC_FHM_EMPTY}, i);
                (void)memcpy(data_at(h, new_slot), data_at(h, i), h->elem_sz_);
                break; /* continues outer loop */
            }
            /* The other slots data has been swapped and we rehash every
               element for this algorithm so there is no need to write its
               tag to this slot. It's data is in correct location already. */
            assert(prev.v == CCC_FHM_DELETED);
            swap(h->data_, data_at(h, i), data_at(h, new_slot), h->elem_sz_);
        } while (1);
    }
    h->avail_ = allowed_cap - h->sz_;
}

static ccc_result
rehash_resize(struct ccc_fhmap_ *const h, size_t const to_add)
{
    assert(((h->mask_ + 1) & h->mask_) == 0);
    size_t const new_pow2_cap = next_power_of_two(h->mask_ + 1 + to_add) << 1;
    if (new_pow2_cap < (h->mask_ + 1))
    {
        return CCC_RESULT_MEM_ERROR;
    }
    size_t const prev_bytes = mask_to_total_bytes(h->elem_sz_, h->mask_);
    size_t const total_bytes
        = mask_to_total_bytes(h->elem_sz_, new_pow2_cap - 1);
    if (total_bytes < prev_bytes)
    {
        return CCC_RESULT_MEM_ERROR;
    }
    void *const new_buf = h->alloc_fn_(NULL, total_bytes, h->aux_);
    if (!new_buf)
    {
        return CCC_RESULT_MEM_ERROR;
    }
    struct ccc_fhmap_ new_h = *h;
    new_h.sz_ = 0;
    new_h.mask_ = new_pow2_cap - 1;
    new_h.avail_ = mask_to_load_factor_cap(new_h.mask_);
    new_h.data_ = new_buf;
    /* Our static assertions at start of file guarantee this is correct. */
    new_h.tag_
        = (ccc_fhm_tag *)((char *)new_buf
                          + mask_to_data_bytes(new_h.elem_sz_, new_h.mask_));
    (void)memset(new_h.tag_, CCC_FHM_EMPTY, mask_to_tag_bytes(new_h.mask_));
    for (size_t i = 0; i < (h->mask_ + 1); ++i)
    {
        if (is_full(h->tag_[i]))
        {
            uint64_t const hash = h->hash_fn_(
                (ccc_user_key){.user_key = key_at(h, i), .aux = h->aux_});
            size_t const new_i = find_known_insert_slot(&new_h, hash);
            set_tag(&new_h, to_tag(hash), new_i);
            (void)memcpy(data_at(&new_h, new_i), data_at(h, i), new_h.elem_sz_);
        }
    }
    new_h.avail_ -= h->sz_;
    new_h.sz_ = h->sz_;
    (void)h->alloc_fn_(h->data_, 0, h->aux_);
    *h = new_h;
    return CCC_RESULT_OK;
}

static inline void
set_tag(struct ccc_fhmap_ *const h, ccc_fhm_tag const m, size_t const i)
{
    size_t const replica_byte
        = ((i - CCC_FHM_GROUP_SIZE) & h->mask_) + CCC_FHM_GROUP_SIZE;
    h->tag_[i] = m;
    h->tag_[replica_byte] = m;
}

static inline ccc_tribool
is_full(ccc_fhm_tag const m)
{
    return (m.v & TAG_MSB) == 0;
}

static inline ccc_tribool
is_constant(ccc_fhm_tag const m)
{
    return (m.v & TAG_MSB) != 0;
}

static inline ccc_fhm_tag
to_tag(uint64_t const hash)
{
    return (ccc_fhm_tag){(hash >> ((sizeof(hash) * CHAR_BIT) - 7))
                         & LOWER_7_BITS_MASK};
}

static inline void *
key_at(struct ccc_fhmap_ const *const h, size_t const i)
{
    assert(i <= h->mask_);
    return (char *)(h->tag_ - ((i + 1) * h->elem_sz_)) + h->key_offset_;
}

static inline void *
data_at(struct ccc_fhmap_ const *const h, size_t const i)
{
    assert(i <= h->mask_);
    return h->tag_ - ((i + 1) * h->elem_sz_);
}

static inline ccc_ucount
data_i(struct ccc_fhmap_ const *const h, void const *const data_slot)
{
    if ((char *)data_slot >= (char *)h->tag_
        || (char *)data_slot <= (char *)h->data_)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count
                        = ((char *)h->tag_ - (char *)data_slot) / h->elem_sz_};
}

static inline void *
swap_slot(struct ccc_fhmap_ const *h)
{
    return h->data_;
}

static inline void
swap(char tmp[const], void *const a, void *const b, size_t const ab_size)
{
    if (unlikely(!a || !b || a == b))
    {
        return;
    }
    (void)memcpy(tmp, a, ab_size);
    (void)memcpy(a, b, ab_size);
    (void)memcpy(b, tmp, ab_size);
}

static inline void *
key_in_slot(struct ccc_fhmap_ const *const h, void const *const slot)
{
    return (char *)slot + h->key_offset_;
}

static inline size_t
next_power_of_two(size_t const n)
{
    if (n <= 1)
    {
        return n + 1;
    }
    unsigned const shifts = countl_0_size_t(n - 1);
    return shifts >= sizeof(size_t) * CHAR_BIT ? 0 : (SIZE_MAX >> shifts) + 1;
}

static inline ccc_tribool
is_power_of_two(size_t const n)
{
    return n && ((n & (n - 1)) == 0);
}

/** Returns the total bytes used by the map in the contiguous allocation. This
includes the bytes for the user data array (swap slot included) and the tag
array. The tag array also has an duplicate group at the end that must be
counted. */
static inline size_t
mask_to_total_bytes(size_t const elem_size, size_t const mask)
{
    if (!elem_size || !mask)
    {
        return 0;
    }
    /* Add two to mask at first due to swap slot. */
    return ((mask + 2) * elem_size) + ((mask + 1) + CCC_FHM_GROUP_SIZE);
}

/** Returns the bytes needed for the tag metadata array. This includes the
bytes for the duplicate group that is at the end of the tag array. */
static inline size_t
mask_to_tag_bytes(size_t const mask)
{
    static_assert(sizeof(ccc_fhm_tag) == sizeof(uint8_t));
    if (!mask)
    {
        return 0;
    }
    return mask + 1 + CCC_FHM_GROUP_SIZE;
}

static inline size_t
mask_to_load_factor_cap(size_t const mask)
{
    return ((mask + 1) / 8) * 7;
}

static inline size_t
mask_to_data_bytes(size_t elem_size, size_t const mask)
{
    /* Add two because there is always a bonus user data type at the 0th index
       of the data array for swapping purposes. */
    return elem_size * (mask + 2);
}

static inline size_t
max(size_t const a, size_t const b)
{
    return a > b ? a : b;
}

/*=====================   Intrinsics and Generics   =========================*/

/** Below are the implementations of the SIMD or bitwise operations needed to
run a search on multiple entries in the hash table simultaneously. For now,
the only container that will use these operations is this one so there is no
need to break out different headers and sources and clutter the src directory.
x86 is the only platform that gets the full benefit of SIMD. Apple and all
other platforms will get a portable implementation due to concerns over NEON
speed of vectorized instructions. However, loading up groups into a uint64_t is
still good and counts simultaneous operations just not the type that uses CPU
vector lanes for a single instruction. */

/* We can vectorize for x86 only. */
#if defined(__x86_64) && defined(__SSE2__) && !defined(CCC_FHM_PORTABLE)

/*========================  Index Mask Implementations   ====================*/

static inline ccc_tribool
is_index_on(index_mask const m)
{
    return m.v != 0;
}

static inline size_t
lowest_on_index(index_mask const m)
{
    return countr_0(m);
}

[[maybe_unused]] static inline size_t
trailing_zeros(index_mask const m)
{
    return countr_0(m);
}

static inline size_t
leading_zeros(index_mask const m)
{
    return countl_0(m);
}

static inline size_t
next_index(index_mask *const m)
{
    assert(m);
    size_t const index = lowest_on_index(*m);
    m->v &= (m->v - 1);
    return index;
}

/*=========================  Group Implementations   ========================*/

static inline group
load_group(ccc_fhm_tag const *const src)
{
    return (group){_mm_loadu_si128((__m128i *)src)};
}

static inline void
store_group(ccc_fhm_tag *const dst, group const src)
{
    _mm_storeu_si128((__m128i *)dst, src.v);
}

static inline index_mask
match_tag(group const g, ccc_fhm_tag const m)
{
    return (index_mask){
        _mm_movemask_epi8(_mm_cmpeq_epi8(g.v, _mm_set1_epi8((int8_t)m.v)))};
}

static inline index_mask
match_empty(group const g)
{
    return match_tag(g, (ccc_fhm_tag){CCC_FHM_EMPTY});
}

static inline index_mask
match_empty_or_deleted(group const g)
{
    static_assert(sizeof(int) >= sizeof(uint16_t));
    return (index_mask){_mm_movemask_epi8(g.v)};
}

[[maybe_unused]] static inline index_mask
match_full(group const g)
{
    index_mask m = match_empty_or_deleted(g);
    m.v = ~m.v;
    return m;
}

static inline group
make_constants_empty_and_full_deleted(group const g)
{
    __m128i const zero = _mm_setzero_si128();
    __m128i const match_constants = _mm_cmpgt_epi8(zero, g.v);
    return (group){
        _mm_or_si128(match_constants, _mm_set1_epi8((int8_t)CCC_FHM_DELETED))};
}

/*====================  Bit Counting for Index Mask   =======================*/

#    if defined(__has_builtin) && __has_builtin(__builtin_ctz)                 \
        && __has_builtin(__builtin_clz) && __has_builtin(__builtin_clzl)

static_assert(sizeof((index_mask){}.v) <= sizeof(unsigned));

static inline unsigned
countr_0(index_mask const m)
{
    /* Even though the mask is implicitly widened to int width there is
       guaranteed to be a 1 bit between bit index 0 and CCC_FHM_GROUP_SIZE - 1
       if the builtin is called. */
    return m.v ? __builtin_ctz(m.v) : CCC_FHM_GROUP_SIZE;
}

static inline unsigned
countl_0(index_mask const m)
{
    /* In the simd version the index mask is only a uint16_t so the number of
       leading zeros would be inaccurate due to widening. There is only a
       builtin for leading and trailing zeros for int width as smallest. */
    static_assert(sizeof(m.v) * 2 == sizeof(unsigned));
    static_assert(CCC_FHM_GROUP_SIZE * 2UL == sizeof(unsigned) * CHAR_BIT);
    return m.v ? __builtin_clz(((unsigned)m.v) << CCC_FHM_GROUP_SIZE)
               : CCC_FHM_GROUP_SIZE;
}

static inline unsigned
countl_0_size_t(size_t const n)
{
    static_assert(sizeof(size_t) == sizeof(unsigned long));
    return n ? __builtin_clzl(n) : sizeof(size_t) * CHAR_BIT;
}

#    else /* !defined(__has_builtin) || !__has_builtin(__builtin_ctz)          \
        || !__has_builtin(__builtin_clz) || !__has_builtin(__builtin_clzl) */

static inline unsigned
countr_0(index_mask m)
{
    if (!m.v)
    {
        return CCC_FHM_GROUP_SIZE;
    }
    unsigned cnt = 0;
    for (; m.v; cnt += ((m.v & 1U) == 0), m.v >>= 1U)
    {}
    return cnt;
}

static inline unsigned
countl_0(index_mask m)
{
    if (!m.v)
    {
        return CCC_FHM_GROUP_SIZE;
    }
    unsigned mv = (unsigned)m.v << CCC_FHM_GROUP_SIZE;
    unsigned cnt = 0;
    for (; (mv & (INDEX_MASK_MSB << CCC_FHM_GROUP_SIZE)) == 0; ++cnt, mv <<= 1U)
    {}
    return cnt;
}

static inline unsigned
countl_0_size_t(size_t n)
{
    if (!n)
    {
        return sizeof(size_t) * CHAR_BIT;
    }
    unsigned cnt = 0;
    for (; !(n & SIZE_T_MSB); ++cnt, n <<= 1U)
    {}
    return cnt;
}

#    endif /* defined(__has_builtin) && __has_builtin(__builtin_ctz)           \
        && __has_builtin(__builtin_clz) && __has_builtin(__builtin_clzl) */

#else /* !defined(__x86_64) || !defined(__SSE2__) ||                           \
         !defined(CCC_FHM_PORTABLE) */

/* What follows is the generic portable implementation when high width SIMD
can't be achieved. For now this means NEON on Apple defaults to generic but
this will likely change in the future as NEON improves. */

/*==========================    Bit Helpers    ==============================*/

#    if defined(__has_builtin) && __has_builtin(__builtin_ctzl)                \
        && __has_builtin(__builtin_clzl)

static_assert(sizeof((index_mask){}.v) == sizeof(long));

static inline unsigned
countr_0(index_mask const m)
{
    return m.v ? __builtin_ctzl(m.v) / CCC_FHM_GROUP_SIZE : CCC_FHM_GROUP_SIZE;
}

static inline unsigned
countl_0(index_mask const m)
{
    return m.v ? __builtin_clzl(m.v) / CCC_FHM_GROUP_SIZE : CCC_FHM_GROUP_SIZE;
}

static inline unsigned
countl_0_size_t(size_t const n)
{
    static_assert(sizeof(size_t) == sizeof(unsigned long));
    return n ? __builtin_clzl(n) : sizeof(size_t) * CHAR_BIT;
}

#    else /* defined(__has_builtin) && __has_builtin(__builtin_ctzl) &&        \
             __has_builtin(__builtin_clzl) */

static inline unsigned
countr_0(index_mask m)
{
    if (!m.v)
    {
        return CCC_FHM_GROUP_SIZE;
    }
    unsigned cnt = 0;
    for (; m.v; cnt += ((m.v & 1U) == 0), m.v >>= 1U)
    {}
    return cnt / CCC_FHM_GROUP_SIZE;
}

static inline unsigned
countl_0(index_mask m)
{
    if (!m.v)
    {
        return CCC_FHM_GROUP_SIZE;
    }
    unsigned cnt = 0;
    for (; (m.v & INDEX_MASK_MSB) == 0; ++cnt, m.v <<= 1U)
    {}
    return cnt / CCC_FHM_GROUP_SIZE;
}

static inline unsigned
countl_0_size_t(size_t n)
{
    if (!n)
    {
        return sizeof(size_t) * CHAR_BIT;
    }
    unsigned cnt = 0;
    for (; (n & SIZE_T_MSB) == 0; ++cnt, n <<= 1U)
    {}
    return cnt;
}

#    endif /* !defined(__has_builtin) || !__has_builtin(__builtin_ctzl) ||     \
              !__has_builtin(__builtin_clzl) */

/* Returns 1 aka true if platform is little endian otherwise false for big
endian. */
static inline int
is_little_endian(void)
{
    unsigned int x = 1;
    char *c = (char *)&x;
    return (int)*c;
}

static inline index_mask
to_little_endian(index_mask m)
{
    if (is_little_endian())
    {
        return m;
    }
#    if defined(__has_builtin) && __has_builtin(__builtin_bswap64)
    m.v = __builtin_bswap64(m.v);
#    else
    m.v = (m.v & 0x00000000FFFFFFFF) << 32 | (m.v & 0xFFFFFFFF00000000) >> 32;
    m.v = (m.v & 0x0000FFFF0000FFFF) << 16 | (m.v & 0xFFFF0000FFFF0000) >> 16;
    m.v = (m.v & 0x00FF00FF00FF00FF) << 8 | (m.v & 0xFF00FF00FF00FF00) >> 8;
#    endif
    return m;
}

/*========================  Index Mask Implementations   ====================*/

static inline ccc_tribool
is_index_on(index_mask const m)
{
    return m.v != 0;
}

static inline size_t
lowest_on_index(index_mask const m)
{
    return countr_0(m);
}

static inline size_t
trailing_zeros(index_mask const m)
{
    return countr_0(m);
}

static inline size_t
leading_zeros(index_mask const m)
{
    return countl_0(m);
}

static inline size_t
next_index(index_mask *const m)
{
    assert(m);
    size_t const index = lowest_on_index(*m);
    m->v &= (m->v - 1);
    return index;
}

/*=========================  Group Implementations   ========================*/

static inline group
load_group(ccc_fhm_tag const *const src)
{
    group g;
    (void)memcpy(&g, src, sizeof(g));
    return g;
}

static inline void
store_group(ccc_fhm_tag *const dst, group const src)
{
    (void)memcpy(dst, &src, sizeof(src));
}

static inline index_mask
match_tag(group g, ccc_fhm_tag const m)
{
    group const cmp = {g.v
                       ^ ((((typeof(g.v))m.v) << (TAG_BITS * 7UL))
                          | (((typeof(g.v))m.v) << (TAG_BITS * 6UL))
                          | (((typeof(g.v))m.v) << (TAG_BITS * 5UL))
                          | (((typeof(g.v))m.v) << (TAG_BITS * 4UL))
                          | (((typeof(g.v))m.v) << (TAG_BITS * 3UL))
                          | (((typeof(g.v))m.v) << (TAG_BITS * 2UL))
                          | (((typeof(g.v))m.v) << TAG_BITS) | m.v)};
    return to_little_endian(
        (index_mask){(cmp.v - GROUP_LSB) & ~cmp.v & GROUP_DELETED});
}

static inline index_mask
match_empty(group const g)
{
    return to_little_endian((index_mask){g.v & (g.v << 1) & GROUP_DELETED});
}

static inline index_mask
match_empty_or_deleted(group const g)
{
    return to_little_endian((index_mask){g.v & GROUP_DELETED});
}

static inline index_mask
match_full(group const g)
{
    index_mask res = match_empty_or_deleted(g);
    res.v = ~res.v;
    return res;
}

static inline group
make_constants_empty_and_full_deleted(group g)
{
    g.v = ~g.v & GROUP_DELETED;
    g.v = ~g.v + (g.v >> (TAG_BITS - 1));
    return g;
}

#endif /* defined(__x86_64) && defined(__SSE2__) */
