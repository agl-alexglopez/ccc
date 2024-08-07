#ifndef PQUEUE
#define PQUEUE

#include <stdbool.h>
#include <stddef.h>
/* NOLINTNEXTLINE */
#include <stdint.h>

/* Standard C-style three way comparison for priority queue elements. */
typedef enum ccc_pq_threeway_cmp
{
    CCC_PQ_LES = -1,
    CCC_PQ_EQL,
    CCC_PQ_GRT,
} ccc_pq_threeway_cmp;

/* The embedded struct type for operation of the priority queue. The priority
   queue does not use the heap so it is the users responsibility to decide
   where elements are allocated in memory. For example:
       struct val
       {
           int val;
           ccc_pq_elem elem;
       };
   Do not access the fields of the struct directly. */
typedef struct ccc_pq_elem
{
    struct ccc_pq_elem *left_child;
    struct ccc_pq_elem *next_sibling;
    struct ccc_pq_elem *prev_sibling;
    struct ccc_pq_elem *parent;
} ccc_pq_elem;

/* Signature for a user defined comparison function between two user defined
   structs. Given two valid pointers to elements in the priority queue and any
   auxilliary data necessary for comparison, return the resulting three way
   comparison for the user struct values. */
typedef ccc_pq_threeway_cmp ccc_pq_cmp_fn(ccc_pq_elem const *,
                                          ccc_pq_elem const *, void *);

/* A function type to aid in deallocation of the priority queue. The user may
   define a destructor function that will act on each element for clearing
   the priority queue. For example, freeing heap allocated structs with
   priority queue handles embedded within them is a common use for such
   a function. */
typedef void ccc_pq_destructor_fn(ccc_pq_elem *);

/* A function type to aid in the update, increase, and decrease operations
   in the priority queue. The new value should be placed in void * argument.
   The user can then cast and use the value referenced in the void * argument
   as they see fit. When the user defines a function to match this signature
   casting the void * should be well defined because the type corresponds to
   the type being used for comparisons in the priority queue. */
typedef void ccc_pq_update_fn(ccc_pq_elem *, void *);

/* The structure used to manage the data in a priority queue. Stack allocation
   is recommended for easy cleanup and speed. However, this structure may be
   placed anywhere that is convenient for the user. Consider the fields
   private. This structure can be initialized upon declaration witht the
   provided initialization macro. */
typedef struct ccc_pqueue
{
    ccc_pq_elem *root;
    size_t sz;
    ccc_pq_cmp_fn *cmp;
    ccc_pq_threeway_cmp order;
    void *aux;
} ccc_pqueue;

/* Given the address of the ccc_pq_elem the user has access to, the name of the
   user defined struct in which the ccc_pq_elem is embedded and the name of the
   field in said struct where the  ccc_pq_elem is embedded, obtains the wrapping
   struct. */
#define CCC_PQ_OF(pq_elem, struct, member)                                     \
    ((struct *)((uint8_t *)&(pq_elem)->parent                                  \
                - offsetof(struct, member.parent))) /* NOLINT */

/* Given the desired total order of the priority queue, the comparison function,
   and any auxilliarry data needed for comparison, initialize an empty priority
   queue on the right hand side of it's declaration. This can be used at
   compile time or runtime. For example:
     ccc_pqueue my_pq = CCC_PQ_INIT(CCC_PQ_LES, my_cmp_fn, NULL);
   Such initialization must always occur or use of the priority queue is
   undefined. */
#define CCC_PQ_INIT(ORDER, CMP_FN, AUX)                                        \
    {                                                                          \
        .root = NULL, .sz = 0, .cmp = (CMP_FN), .order = (ORDER), .aux = (AUX) \
    }

/* Obtain a reference to the front of the priority queue. This will be a min
   or max depending on the initialization of the priority queue. O(1). */
ccc_pq_elem const *ccc_pq_front(ccc_pqueue const *);

/* Adds an element to the priority queue in correct total order. O(1). */
void ccc_pq_push(ccc_pqueue *, ccc_pq_elem *);

/* Pops the front element from the priority queue. O(lgN). */
ccc_pq_elem *ccc_pq_pop(ccc_pqueue *);

/* Erase the specified element from the priority queue. This need not be
   the front element. O(lgN). */
ccc_pq_elem *ccc_pq_erase(ccc_pqueue *, ccc_pq_elem *);

/* Returns true if the priority queue is empty false if not. */
bool ccc_pq_empty(ccc_pqueue const *);

/* Returns the size of the priority queue. */
size_t ccc_pq_size(ccc_pqueue const *);

/* Update the value of a priority queue element if the new value is not
   known to be less than or greater than the old value. This operation
   may incur uneccessary overhead if the user can deduce if an increase
   or decrease is occuring. See the increase and decrease operations. O(1)
   best case, O(lgN) worst case. */
bool ccc_pq_update(ccc_pqueue *, ccc_pq_elem *, ccc_pq_update_fn *, void *);

/* Optimal update technique if the priority queue has been initialized as
   a max queue and the new value is known to be greater than the old value.
   If this is a max heap O(1), otherwise O(lgN). */
bool ccc_pq_increase(ccc_pqueue *, ccc_pq_elem *, ccc_pq_update_fn *, void *);

/* Optimal update technique if the priority queue has been initialized as
   a min queue and the new value is known to be less than the old value.
   If this is a min heap O(1), otherwise O(lgN). */
bool ccc_pq_decrease(ccc_pqueue *, ccc_pq_elem *, ccc_pq_update_fn *, void *);

/* Return the order used to initialize the heap. */
ccc_pq_threeway_cmp ccc_pq_order(ccc_pqueue const *);

/* Calls the user provided destructor on each element in the priority queue.
   It is safe to free the struct if it has been heap allocated as elements
   are popped from the priority queue before the function is called. O(NlgN). */
void ccc_pq_clear(ccc_pqueue *, ccc_pq_destructor_fn *);

/* Internal validation function for the state of the heap. This should be of
   little interest to the user. */
bool ccc_pq_validate(ccc_pqueue const *);

#endif /* PQUEUE */
