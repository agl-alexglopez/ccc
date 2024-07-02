#ifndef PQUEUE
#define PQUEUE

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum pq_threeway_cmp
{
    PQLES = -1,
    PQEQL,
    PQGRT,
};

struct pq_elem
{
    struct pq_elem *left_child;
    struct pq_elem *next_sibling;
    struct pq_elem *prev_sibling;
    struct pq_elem *parent;
};

typedef enum pq_threeway_cmp ppq_cmp_fn(const struct pq_elem *,
                                        const struct pq_elem *, void *);

typedef void pq_destructor_fn(struct pq_elem *);

typedef void pq_update_fn(struct pq_elem *, void *);

struct pqueue
{
    struct pq_elem *root;
    size_t sz;
    ppq_cmp_fn *cmp;
    enum pq_threeway_cmp order;
    void *aux;
};

#define PQ_ENTRY(PPQ_ELEM, STRUCT, MEMBER)                                     \
    ((STRUCT *)((uint8_t *)&(PPQ_ELEM)->parent                                 \
                - offsetof(STRUCT, MEMBER.parent))) /* NOLINT */

#define PQ_INIT(ORDER, CMP_FN, AUX)                                            \
    {                                                                          \
        .root = NULL, .sz = 0, .cmp = (CMP_FN), .order = (ORDER), .aux = (AUX) \
    }

const struct pq_elem *pq_front(const struct pqueue *);
void pq_push(struct pqueue *, struct pq_elem *);
struct pq_elem *pq_pop(struct pqueue *);
struct pq_elem *pq_erase(struct pqueue *, struct pq_elem *);
void pq_clear(struct pqueue *, pq_destructor_fn *);
bool pq_empty(const struct pqueue *);
size_t pq_size(const struct pqueue *);
bool pq_update(struct pqueue *, struct pq_elem *, pq_update_fn *, void *);
bool pq_increase(struct pqueue *, struct pq_elem *, pq_update_fn *, void *);
bool pq_decrease(struct pqueue *, struct pq_elem *, pq_update_fn *, void *);
bool pq_validate(const struct pqueue *);
enum pq_threeway_cmp pq_order(const struct pqueue *);

#endif /* PQUEUE */
