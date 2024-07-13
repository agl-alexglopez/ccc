#ifndef PQUEUE
#define PQUEUE

#include <stdbool.h>
#include <stddef.h>
/* NOLINTNEXTLINE */
#include <stdint.h>

/* Standard C-style three way comparison for priority queue elements. */
enum pq_threeway_cmp
{
    PQLES = -1,
    PQEQL,
    PQGRT,
};

/* The embedded struct type for operation of the priority queue. The priority
   queue does not use the heap so it is the users responsibility to decide
   where elements are allocated in memory. For example:
       struct val
       {
           int val;
           struct pq_elem elem;
       };
   Do not access the fields of the struct directly. */
struct pq_elem
{
    struct pq_elem *left_child;
    struct pq_elem *next_sibling;
    struct pq_elem *prev_sibling;
    struct pq_elem *parent;
};

/* Signature for a user defined comparison function between two user defined
   structs. Given two valid pointers to elements in the priority queue and any
   auxilliary data necessary for comparison, return the resulting three way
   comparison for the user struct values. */
typedef enum pq_threeway_cmp pq_cmp_fn(const struct pq_elem *,
                                       const struct pq_elem *, void *);

/* A function type to aid in deallocation of the priority queue. The user may
   define a destructor function that will act on each element for clearing
   the priority queue. For example, freeing heap allocated structs with
   priority queue handles embedded within them is a common use for such
   a function. */
typedef void pq_destructor_fn(struct pq_elem *);

/* A function type to aid in the update, increase, and decrease operations
   in the priority queue. The new value should be placed in void * argument.
   The user can then cast and use the value referenced in the void * argument
   as they see fit. When the user defines a function to match this signature
   casting the void * should be well defined because the type corresponds to
   the type being used for comparisons in the priority queue. */
typedef void pq_update_fn(struct pq_elem *, void *);

/* The structure used to manage the data in a priority queue. Stack allocation
   is recommended for easy cleanup and speed. However, this structure may be
   placed anywhere that is convenient for the user. Consider the fields
   private. This structure can be initialized upon declaration witht the
   provided initialization macro. */
struct pqueue
{
    struct pq_elem *root;
    size_t sz;
    pq_cmp_fn *cmp;
    enum pq_threeway_cmp order;
    void *aux;
};

/* Given the address of the pq_elem the user has access to, the name of the
   user defined struct in which the pq_elem is embedded and the name of the
   field in said struct where the  pq_elem is embedded, obtains the wrapping
   struct. */
#define PQ_ENTRY(PQ_ELEM, STRUCT, MEMBER)                                      \
    ((STRUCT *)((uint8_t *)&(PQ_ELEM)->parent                                  \
                - offsetof(STRUCT, MEMBER.parent))) /* NOLINT */

/* Given the desired total order of the priority queue, the comparison function,
   and any auxilliarry data needed for comparison, initialize an empty priority
   queue on the right hand side of it's declaration. This can be used at
   compile time or runtime. For example:
     struct pqueue my_pq = PQ_INIT(PQLES, my_cmp_fn, NULL);
   Such initialization must always occur or use of the priority queue is
   undefined. */
#define PQ_INIT(ORDER, CMP_FN, AUX)                                            \
    {                                                                          \
        .root = NULL, .sz = 0, .cmp = (CMP_FN), .order = (ORDER), .aux = (AUX) \
    }

/* Obtain a reference to the front of the priority queue. This will be a min
   or max depending on the initialization of the priority queue. O(1). */
const struct pq_elem *pq_front(const struct pqueue *);

/* Adds an element to the priority queue in correct total order. O(1). */
void pq_push(struct pqueue *, struct pq_elem *);

/* Pops the front element from the priority queue. O(lgN). */
struct pq_elem *pq_pop(struct pqueue *);

/* Erase the specified element from the priority queue. This need not be
   the front element. O(lgN). */
struct pq_elem *pq_erase(struct pqueue *, struct pq_elem *);

/* Returns true if the priority queue is empty false if not. */
bool pq_empty(const struct pqueue *);

/* Returns the size of the priority queue. */
size_t pq_size(const struct pqueue *);

/* Update the value of a priority queue element if the new value is not
   known to be less than or greater than the old value. This operation
   may incur uneccessary overhead if the user can deduce if an increase
   or decrease is occuring. See the increase and decrease operations. O(1)
   best case, O(lgN) worst case. */
bool pq_update(struct pqueue *, struct pq_elem *, pq_update_fn *, void *);

/* Optimal update technique if the priority queue has been initialized as
   a max queue and the new value is known to be greater than the old value.
   If this is a max heap O(1), otherwise O(lgN). */
bool pq_increase(struct pqueue *, struct pq_elem *, pq_update_fn *, void *);

/* Optimal update technique if the priority queue has been initialized as
   a min queue and the new value is known to be less than the old value.
   If this is a min heap O(1), otherwise O(lgN). */
bool pq_decrease(struct pqueue *, struct pq_elem *, pq_update_fn *, void *);

/* Return the order used to initialize the heap. */
enum pq_threeway_cmp pq_order(const struct pqueue *);

/* Calls the user provided destructor on each element in the priority queue.
   It is safe to free the struct if it has been heap allocated as elements
   are popped from the priority queue before the function is called. O(NlgN). */
void pq_clear(struct pqueue *, pq_destructor_fn *);

/* Internal validation function for the state of the heap. This should be of
   little interest to the user. */
bool pq_validate(const struct pqueue *);

#endif /* PQUEUE */
