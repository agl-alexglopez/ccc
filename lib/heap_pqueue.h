#ifndef HEAP_PQUEUE
#define HEAP_PQUEUE

#include "attrib.h"

#include <stdbool.h>
#include <stddef.h>
/* NOLINTNEXTLINE */
#include <stdint.h>

enum heap_ccc_pq_threeway_cmp
{
    HPQLES = -1,
    HPQEQL,
    HPQGRT,
};

struct hccc_pq_elem
{
    size_t handle;
};

typedef enum heap_ccc_pq_threeway_cmp
hpq_cmp_fn(struct hccc_pq_elem const *, struct hccc_pq_elem const *, void *);

typedef void hpq_destructor_fn(struct hccc_pq_elem *);

typedef void hpq_update_fn(struct hccc_pq_elem *, void *);

typedef void hpq_print_fn(struct hccc_pq_elem const *);

struct heap_pqueue
{
    struct hccc_pq_elem **heap ATTRIB_PRIVATE;
    size_t sz ATTRIB_PRIVATE;
    size_t capacity ATTRIB_PRIVATE;
    hpq_cmp_fn *cmp ATTRIB_PRIVATE;
    enum heap_ccc_pq_threeway_cmp order ATTRIB_PRIVATE;
    void *aux ATTRIB_PRIVATE;
};

#define HPQ_ENTRY(Hccc_pq_elem, STRUCT, MEMBER)                                \
    ((STRUCT *)((uint8_t *)&(Hccc_pq_elem)->handle                             \
                - offsetof(STRUCT, MEMBER.handle))) /* NOLINT */

void hpq_init(struct heap_pqueue *, enum heap_ccc_pq_threeway_cmp hpq_ordering,
              hpq_cmp_fn *, void *);
struct hccc_pq_elem const *hpq_front(struct heap_pqueue const *);
void hpq_push(struct heap_pqueue *, struct hccc_pq_elem *);
struct hccc_pq_elem *hpq_pop(struct heap_pqueue *);
struct hccc_pq_elem *hpq_erase(struct heap_pqueue *, struct hccc_pq_elem *);
void hpq_clear(struct heap_pqueue *, hpq_destructor_fn *);
bool hpq_empty(struct heap_pqueue const *);
size_t hpq_size(struct heap_pqueue const *);
bool hpq_update(struct heap_pqueue *, struct hccc_pq_elem *, hpq_update_fn *,
                void *);
bool hpq_validate(struct heap_pqueue const *);
enum heap_ccc_pq_threeway_cmp hpq_order(struct heap_pqueue const *);

void hpq_print(struct heap_pqueue const *, size_t, hpq_print_fn *);

#endif
