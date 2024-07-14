#ifndef HEAP_PQUEUE
#define HEAP_PQUEUE

#include "attrib.h"

#include <stdbool.h>
#include <stddef.h>
/* NOLINTNEXTLINE */
#include <stdint.h>

enum heap_pq_threeway_cmp
{
    HPQLES = -1,
    HPQEQL,
    HPQGRT,
};

struct hpq_elem
{
    size_t handle;
};

typedef enum heap_pq_threeway_cmp hpq_cmp_fn(struct hpq_elem const *,
                                             struct hpq_elem const *, void *);

typedef void hpq_destructor_fn(struct hpq_elem *);

typedef void hpq_update_fn(struct hpq_elem *, void *);

typedef void hpq_print_fn(struct hpq_elem const *);

struct heap_pqueue
{
    struct hpq_elem **heap ATTRIB_PRIVATE;
    size_t sz ATTRIB_PRIVATE;
    size_t capacity ATTRIB_PRIVATE;
    hpq_cmp_fn *cmp ATTRIB_PRIVATE;
    enum heap_pq_threeway_cmp order ATTRIB_PRIVATE;
    void *aux ATTRIB_PRIVATE;
};

#define HPQ_ENTRY(HPQ_ELEM, STRUCT, MEMBER)                                    \
    ((STRUCT *)((uint8_t *)&(HPQ_ELEM)->handle                                 \
                - offsetof(STRUCT, MEMBER.handle))) /* NOLINT */

void hpq_init(struct heap_pqueue *, enum heap_pq_threeway_cmp hpq_ordering,
              hpq_cmp_fn *, void *);
struct hpq_elem const *hpq_front(struct heap_pqueue const *);
void hpq_push(struct heap_pqueue *, struct hpq_elem *);
struct hpq_elem *hpq_pop(struct heap_pqueue *);
struct hpq_elem *hpq_erase(struct heap_pqueue *, struct hpq_elem *);
void hpq_clear(struct heap_pqueue *, hpq_destructor_fn *);
bool hpq_empty(struct heap_pqueue const *);
size_t hpq_size(struct heap_pqueue const *);
bool hpq_update(struct heap_pqueue *, struct hpq_elem *, hpq_update_fn *,
                void *);
bool hpq_validate(struct heap_pqueue const *);
enum heap_pq_threeway_cmp hpq_order(struct heap_pqueue const *);

void hpq_print(struct heap_pqueue const *, size_t, hpq_print_fn *);

#endif
