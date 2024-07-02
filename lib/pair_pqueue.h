#ifndef PAIR_PQUEUE
#define PAIR_PQUEUE

#include "attrib.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum ppq_threeway_cmp
{
    PPQLES = -1,
    PPQEQL,
    PPQGRT,
};

struct ppq_elem
{
    struct ppq_elem *left_child;
    struct ppq_elem *next_sibling;
    struct ppq_elem *prev_sibling;
    struct ppq_elem *parent;
};

typedef enum ppq_threeway_cmp ppq_cmp_fn(const struct ppq_elem *,
                                         const struct ppq_elem *, void *);

typedef void ppq_destructor_fn(struct ppq_elem *);

typedef void ppq_update_fn(struct ppq_elem *, void *);

struct pair_pqueue
{
    struct ppq_elem *root;
    size_t sz;
    ppq_cmp_fn *cmp;
    enum ppq_threeway_cmp order;
    void *aux;
};

#define PPQ_ENTRY(PPQ_ELEM, STRUCT, MEMBER)                                    \
    ((STRUCT *)((uint8_t *)&(PPQ_ELEM)->parent                                 \
                - offsetof(STRUCT, MEMBER.parent))) /* NOLINT */

#define PPQ_INIT(ORDER, CMP_FN, AUX)                                           \
    {                                                                          \
        .root = NULL, .sz = 0, .cmp = (CMP_FN), .order = (ORDER), .aux = (AUX) \
    }

const struct ppq_elem *ppq_front(const struct pair_pqueue *);
void ppq_push(struct pair_pqueue *, struct ppq_elem *);
struct ppq_elem *ppq_pop(struct pair_pqueue *);
struct ppq_elem *ppq_erase(struct pair_pqueue *, struct ppq_elem *);
void ppq_clear(struct pair_pqueue *, ppq_destructor_fn *);
bool ppq_empty(const struct pair_pqueue *);
size_t ppq_size(const struct pair_pqueue *);
bool ppq_update(struct pair_pqueue *, struct ppq_elem *, ppq_update_fn *,
                void *);
bool ppq_increase(struct pair_pqueue *, struct ppq_elem *, ppq_update_fn *,
                  void *);
bool ppq_decrease(struct pair_pqueue *, struct ppq_elem *, ppq_update_fn *,
                  void *);
bool ppq_validate(const struct pair_pqueue *);
enum ppq_threeway_cmp ppq_order(const struct pair_pqueue *);

#endif /* PAIR_PQUEUE */
