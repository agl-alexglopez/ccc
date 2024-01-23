#ifndef TREE
#define TREE
#include <stdbool.h>
#include <stddef.h>

enum tree_link
{
    L = 0,
    R = 1
};

enum list_link
{
    P = 0,
    N = 1
};

struct node
{
    struct node *link[2];
    struct node *parent_or_dups;
};

struct tree
{
    struct node *root;
    struct node end;
    size_t size;
};

typedef enum
{
    LES = -1,
    EQL = 0,
    GRT = 1
} threeway_cmp;

/* To implement three way comparison in C you can try something
   like this:

     return (a > b) - (a < b);

   If such a comparison is not possible for your type you can simply
   return the value of the cmp enum directly with conditionals switch
   statements or whatever other comparison logic you choose. */
typedef threeway_cmp tree_cmp_fn(const struct node *key, const struct node *n,
                                 void *aux);

/* Mostly intended for debugging. Validates the underlying tree
   data structure with invariants that must hold regardless of
   interface. */
bool validate_tree(struct tree *t, tree_cmp_fn *cmp);

/* Use this function in gdb or a terminal for some pretty colors.
   Intended for debuggin use. */
void print_tree(struct tree *t, const struct node *root);

#endif
