#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#define TRAITS_USING_NAMESPACE_CCC
#define TREE_MAP_USING_NAMESPACE_CCC

#include "checkers.h"
#include "traits.h"
#include "tree_map.h"
#include "tree_map_utility.h"
#include "types.h"
#include "utility/stack_allocator.h"

static inline struct Val
tree_map_create(int const id, int const val)
{
    return (struct Val){.key = id, .val = val};
}

static inline void
tree_map_modplus(CCC_Type_context const t)
{
    ((struct Val *)t.type)->val++;
}

check_static_begin(tree_map_test_insert)
{
    CCC_Tree_map rom
        = tree_map_initialize(struct Val, elem, key, id_order, NULL, NULL);

    /* Nothing was there before so nothing is in the entry. */
    CCC_Entry ent = swap_entry(&rom, &(struct Val){.key = 137, .val = 99}.elem,
                               &(struct Val){}.elem);
    check(occupied(&ent), false);
    check(unwrap(&ent), NULL);
    check(count(&rom).count, 1);
    check_end();
}

check_static_begin(tree_map_test_insert_macros)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 10);
    CCC_Tree_map rom = tree_map_initialize(
        struct Val, elem, key, id_order, stack_allocator_allocate, &allocator);

    struct Val const *ins = CCC_tree_map_or_insert_with(
        entry_wrap(&rom, &(int){2}), (struct Val){.key = 2, .val = 0});
    check(ins != NULL, true);
    check(validate(&rom), true);
    check(count(&rom).count, 1);
    ins = tree_map_insert_entry_with(entry_wrap(&rom, &(int){2}),
                                     (struct Val){.key = 2, .val = 0});
    check(validate(&rom), true);
    check(ins != NULL, true);
    ins = tree_map_insert_entry_with(entry_wrap(&rom, &(int){9}),
                                     (struct Val){.key = 9, .val = 1});
    check(validate(&rom), true);
    check(ins != NULL, true);
    ins = CCC_entry_unwrap(
        tree_map_insert_or_assign_with(&rom, 3, (struct Val){.val = 99}));
    check(validate(&rom), true);
    check(ins == NULL, false);
    check(validate(&rom), true);
    check(ins->val, 99);
    check(count(&rom).count, 3);
    ins = CCC_entry_unwrap(
        tree_map_insert_or_assign_with(&rom, 3, (struct Val){.val = 98}));
    check(validate(&rom), true);
    check(ins == NULL, false);
    check(ins->val, 98);
    check(count(&rom).count, 3);
    ins = CCC_entry_unwrap(
        tree_map_try_insert_with(&rom, 3, (struct Val){.val = 100}));
    check(ins == NULL, false);
    check(validate(&rom), true);
    check(ins->val, 98);
    check(count(&rom).count, 3);
    ins = CCC_entry_unwrap(
        tree_map_try_insert_with(&rom, 4, (struct Val){.val = 100}));
    check(ins == NULL, false);
    check(validate(&rom), true);
    check(ins->val, 100);
    check(count(&rom).count, 4);
    check_end(CCC_tree_map_clear(&rom, NULL););
}

check_static_begin(tree_map_test_insert_overwrite)
{
    CCC_Tree_map rom
        = tree_map_initialize(struct Val, elem, key, id_order, NULL, NULL);

    struct Val q = {.key = 137, .val = 99};
    CCC_Entry ent = swap_entry(&rom, &q.elem, &(struct Val){}.elem);
    check(occupied(&ent), false);
    check(unwrap(&ent), NULL);

    struct Val const *v = unwrap(entry_wrap(&rom, &q.key));
    check(v != NULL, true);
    check(v->val, 99);

    /* Now the second insertion will take place and the old occupying value
       will be written into our struct we used to make the query. */
    struct Val r = (struct Val){.key = 137, .val = 100};

    /* The contents of q are now in the table. */
    CCC_Entry old_ent = swap_entry(&rom, &r.elem, &(struct Val){}.elem);
    check(occupied(&old_ent), true);

    /* The old contents are now in q and the entry is in the table. */
    v = unwrap(&old_ent);
    check(v != NULL, true);
    check(v->val, 99);
    check(r.val, 99);
    v = unwrap(entry_wrap(&rom, &r.key));
    check(v != NULL, true);
    check(v->val, 100);
    check_end();
}

check_static_begin(tree_map_test_insert_then_bad_ideas)
{
    CCC_Tree_map rom
        = tree_map_initialize(struct Val, elem, key, id_order, NULL, NULL);
    struct Val q = {.key = 137, .val = 99};
    CCC_Entry ent = swap_entry(&rom, &q.elem, &(struct Val){}.elem);
    check(occupied(&ent), false);
    check(unwrap(&ent), NULL);
    struct Val const *v = unwrap(entry_wrap(&rom, &q.key));
    check(v != NULL, true);
    check(v->val, 99);

    struct Val r = (struct Val){.key = 137, .val = 100};

    ent = swap_entry(&rom, &r.elem, &(struct Val){}.elem);
    check(occupied(&ent), true);
    v = unwrap(&ent);
    check(v != NULL, true);
    check(v->val, 99);
    check(r.val, 99);
    r.val -= 9;

    v = get_key_value(&rom, &q.key);
    check(v != NULL, true);
    check(v->val, 100);
    check(r.val, 90);
    check_end();
}

check_static_begin(tree_map_test_entry_api_functional)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 200);
    CCC_Tree_map rom = tree_map_initialize(
        struct Val, elem, key, id_order, stack_allocator_allocate, &allocator);
    size_t const size = 200;

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    struct Val def = {0};
    for (size_t i = 0; i < size / 2; i += 2)
    {
        def.key = (int)i;
        def.val = (int)i;
        struct Val const *const d
            = or_insert(entry_wrap(&rom, &def.key), &def.elem);
        check((d != NULL), true);
        check(d->key, i);
        check(d->val, i);
    }
    check(count(&rom).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.key = (int)i;
        def.val = (int)i;
        struct Val const *const d = or_insert(
            tree_map_and_modify_with(entry_wrap(&rom, &def.key), struct Val,
                                     {
                                         T->val++;
                                     }),
            &def.elem);
        /* All values in the array should be odd now */
        check((d != NULL), true);
        check(d->key, i);
        if (i % 2)
        {
            check(d->val, i);
        }
        else
        {
            check(d->val, i + 1);
        }
        check(d->val % 2, true);
    }
    check(count(&rom).count, (size / 2));
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.key = (int)i;
        def.val = (int)i;
        struct Val *const in = or_insert(entry_wrap(&rom, &def.key), &def.elem);
        in->val++;
        /* All values in the array should be odd now */
        check((in->val % 2 == 0), true);
    }
    check(count(&rom).count, (size / 2));
    check_end(tree_map_clear(&rom, NULL););
}

check_static_begin(tree_map_test_insert_via_entry)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 200);
    CCC_Tree_map rom = tree_map_initialize(
        struct Val, elem, key, id_order, stack_allocator_allocate, &allocator);
    size_t const size = 200;

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    struct Val def = {};
    for (size_t i = 0; i < size / 2; i += 2)
    {
        def.key = (int)i;
        def.val = (int)i;
        struct Val const *const d
            = insert_entry(entry_wrap(&rom, &def.key), &def.elem);
        check((d != NULL), true);
        check(d->key, i);
        check(d->val, i);
    }
    check(count(&rom).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.key = (int)i;
        def.val = (int)i + 1;
        struct Val const *const d
            = insert_entry(entry_wrap(&rom, &def.key), &def.elem);
        /* All values in the array should be odd now */
        check((d != NULL), true);
        check(d->val, i + 1);
        if (i % 2)
        {
            check(d->val % 2 == 0, true);
        }
        else
        {
            check(d->val % 2, true);
        }
    }
    check(count(&rom).count, (size / 2));
    check_end(tree_map_clear(&rom, NULL););
}

check_static_begin(tree_map_test_insert_via_entry_macros)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 200);
    CCC_Tree_map rom = tree_map_initialize(
        struct Val, elem, key, id_order, stack_allocator_allocate, &allocator);

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (size_t i = 0; i < size / 2; i += 2)
    {
        struct Val const *const d
            = insert_entry(entry_wrap(&rom, &i), &(struct Val){i, i, {}}.elem);
        check((d != NULL), true);
        check(d->key, i);
        check(d->val, i);
    }
    check(count(&rom).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        struct Val const *const d = insert_entry(
            entry_wrap(&rom, &i), &(struct Val){i, i + 1, {}}.elem);
        /* All values in the array should be odd now */
        check((d != NULL), true);
        check(d->val, i + 1);
        if (i % 2)
        {
            check(d->val % 2 == 0, true);
        }
        else
        {
            check(d->val % 2, true);
        }
    }
    check(count(&rom).count, (size / 2));
    check_end(tree_map_clear(&rom, NULL););
}

check_static_begin(tree_map_test_entry_api_macros)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    int const size = 200;
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 200);
    CCC_Tree_map rom = tree_map_initialize(
        struct Val, elem, key, id_order, stack_allocator_allocate, &allocator);

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (int i = 0; i < size / 2; i += 2)
    {
        /* The macros support functions that will only execute if the or
           insert branch executes. */
        struct Val const *const d = tree_map_or_insert_with(
            entry_wrap(&rom, &i), tree_map_create(i, i));
        check((d != NULL), true);
        check(d->key, i);
        check(d->val, i);
    }
    check(count(&rom).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct Val const *const d = tree_map_or_insert_with(
            and_modify(entry_wrap(&rom, &i), tree_map_modplus),
            tree_map_create(i, i));
        /* All values in the array should be odd now */
        check((d != NULL), true);
        check(d->key, i);
        if (i % 2)
        {
            check(d->val, i);
        }
        else
        {
            check(d->val, i + 1);
        }
        check(d->val % 2, true);
    }
    check(count(&rom).count, (size / 2));
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct Val *v
            = tree_map_or_insert_with(entry_wrap(&rom, &i), (struct Val){});
        check(v != NULL, true);
        v->val++;
        /* All values in the array should be odd now */
        check(v->val % 2 == 0, true);
    }
    check(count(&rom).count, (size / 2));
    check_end(tree_map_clear(&rom, NULL););
}

check_static_begin(tree_map_test_two_sum)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 10);
    CCC_Tree_map rom = tree_map_initialize(
        struct Val, elem, key, id_order, stack_allocator_allocate, &allocator);
    int const addends[10] = {1, 3, -980, 6, 7, 13, 44, 32, 995, -1};
    int const target = 15;
    int solution_indices[2] = {-1, -1};
    for (size_t i = 0; i < (size_t)(sizeof(addends) / sizeof(addends[0])); ++i)
    {
        struct Val const *const other_addend
            = get_key_value(&rom, &(int){target - addends[i]});
        if (other_addend)
        {
            solution_indices[0] = (int)i;
            solution_indices[1] = other_addend->val;
            break;
        }
        CCC_Entry const e = insert_or_assign(
            &rom, &(struct Val){.key = addends[i], .val = i}.elem);
        check(insert_error(&e), false);
    }
    check(solution_indices[0], 8);
    check(solution_indices[1], 2);
    check_end(tree_map_clear(&rom, NULL););
}

check_static_begin(tree_map_test_insert_and_find)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 100);
    CCC_Tree_map rom = tree_map_initialize(
        struct Val, elem, key, id_order, stack_allocator_allocate, &allocator);
    int const size = 100;

    for (int i = 0; i < size; i += 2)
    {
        CCC_Entry e = try_insert(&rom, &(struct Val){.key = i, .val = i}.elem);
        check(occupied(&e), false);
        check(validate(&rom), true);
        e = try_insert(&rom, &(struct Val){.key = i, .val = i}.elem);
        check(occupied(&e), true);
        check(validate(&rom), true);
        struct Val const *const v = unwrap(&e);
        check(v == NULL, false);
        check(v->key, i);
        check(v->val, i);
    }
    for (int i = 0; i < size; i += 2)
    {
        check(contains(&rom, &i), true);
        check(occupied(entry_wrap(&rom, &i)), true);
        check(validate(&rom), true);
    }
    for (int i = 1; i < size; i += 2)
    {
        check(contains(&rom, &i), false);
        check(occupied(entry_wrap(&rom, &i)), false);
        check(validate(&rom), true);
    }
    check_end(tree_map_clear(&rom, NULL););
}

check_static_begin(tree_map_test_insert_shuffle)
{
    size_t const size = 50;
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 50);
    CCC_Tree_map rom = tree_map_initialize(
        struct Val, elem, key, id_order, stack_allocator_allocate, &allocator);
    check(size > 1, true);
    int const prime = 53;
    check(insert_shuffled(&rom, size, prime), CHECK_PASS);
    int sorted_check[50];
    check(inorder_fill(sorted_check, size, &rom), CHECK_PASS);
    for (size_t i = 1; i < size; ++i)
    {
        check(sorted_check[i - 1] <= sorted_check[i], true);
    }
    check_end();
}

check_static_begin(tree_map_test_insert_weak_srand)
{
    int const num_nodes = 100;
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 100);
    CCC_Tree_map rom = tree_map_initialize(
        struct Val, elem, key, id_order, stack_allocator_allocate, &allocator);
    srand(time(NULL)); /* NOLINT */
    for (int i = 0; i < num_nodes; ++i)
    {
        CCC_Entry const e = swap_entry(&rom,
                                       &(struct Val){
                                           .key = rand() /* NOLINT */,
                                           .val = i,
                                       }
                                            .elem,
                                       &(struct Val){}.elem);
        check(insert_error(&e), false);
        check(validate(&rom), true);
    }
    check(count(&rom).count, (size_t)num_nodes);
    check_end(tree_map_clear(&rom, NULL););
}

int
main()
{
    return check_run(
        tree_map_test_insert(), tree_map_test_insert_macros(),
        tree_map_test_insert_and_find(), tree_map_test_insert_overwrite(),
        tree_map_test_insert_then_bad_ideas(), tree_map_test_insert_via_entry(),
        tree_map_test_insert_via_entry_macros(),
        tree_map_test_entry_api_functional(), tree_map_test_entry_api_macros(),
        tree_map_test_two_sum(), tree_map_test_insert_weak_srand(),
        tree_map_test_insert_shuffle());
}
