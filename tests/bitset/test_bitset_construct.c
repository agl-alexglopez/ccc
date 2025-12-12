#include <stddef.h>

#include "ccc/bitset.h"
#include "ccc/types.h"
#include "checkers.h"
#include "utility/stack_allocator.h"
#include "utility/string_view/string_view.h"

typedef typeof(*(CCC_Bitset){}.blocks) Bitblocks;

#define to_blocks(bit_count)                                                   \
    (((bit_count) + CCC_BITSET_BLOCK_BITS - 1) / CCC_BITSET_BLOCK_BITS)

check_static_begin(bitset_test_construct)
{
    CCC_Bitset bs
        = CCC_bitset_initialize(CCC_bitset_blocks(10), NULL, NULL, 10);
    check(CCC_bitset_popcount(&bs).count, 0);
    for (size_t i = 0; i < CCC_bitset_capacity(&bs).count; ++i)
    {
        check(CCC_bitset_test(&bs, i), CCC_FALSE);
        check(CCC_bitset_test(&bs, i), CCC_FALSE);
    }
    check_end();
}

check_static_begin(bitset_test_construct_with_literal)
{
    CCC_Bitset bs = CCC_bitset_with_compound_literal(10, CCC_bitset_blocks(10));
    check(CCC_bitset_popcount(&bs).count, 0);
    for (size_t i = 0; i < CCC_bitset_count(&bs).count; ++i)
    {
        check(CCC_bitset_test(&bs, i), CCC_FALSE);
        check(CCC_bitset_test(&bs, i), CCC_FALSE);
    }
    check_end();
}

check_static_begin(bitset_test_copy_no_allocate)
{
    CCC_Bitset source
        = CCC_bitset_initialize(CCC_bitset_blocks(512), NULL, NULL, 512, 0);
    check(CCC_bitset_capacity(&source).count, 512);
    check(CCC_bitset_count(&source).count, 0);
    CCC_Result push_status = CCC_RESULT_OK;
    for (size_t i = 0; push_status == CCC_RESULT_OK; ++i)
    {
        if (i % 2)
        {
            push_status = CCC_bitset_push_back(&source, CCC_TRUE);
        }
        else
        {
            push_status = CCC_bitset_push_back(&source, CCC_FALSE);
        }
    }
    check(push_status, CCC_RESULT_NO_ALLOCATION_FUNCTION);
    CCC_Bitset destination
        = CCC_bitset_initialize(CCC_bitset_blocks(513), NULL, NULL, 513, 0);
    CCC_Result r = CCC_bitset_copy(&destination, &source, NULL);
    check(r, CCC_RESULT_OK);
    check(CCC_bitset_popcount(&source).count,
          CCC_bitset_popcount(&destination).count);
    check(CCC_bitset_count(&source).count,
          CCC_bitset_count(&destination).count);
    while (!CCC_bitset_is_empty(&source) && !CCC_bitset_is_empty(&destination))
    {
        CCC_Tribool const source_msb = CCC_bitset_pop_back(&source);
        CCC_Tribool const destination_msb = CCC_bitset_pop_back(&destination);
        if (CCC_bitset_count(&source).count % 2)
        {
            check(source_msb, CCC_TRUE);
            check(source_msb, destination_msb);
        }
        else
        {
            check(source_msb, CCC_FALSE);
            check(source_msb, destination_msb);
        }
    }
    check(CCC_bitset_is_empty(&source), CCC_bitset_is_empty(&destination));
    check_end();
}

check_static_begin(bitset_test_copy_allocate)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(Bitblocks, to_blocks(1024));
    CCC_Bitset source = CCC_bitset_with_capacity(stack_allocator_allocate,
                                                 &allocator, 512, 0);
    for (size_t i = 0; i < 512; ++i)
    {
        if (i % 2)
        {
            check(CCC_bitset_push_back(&source, CCC_TRUE), CCC_RESULT_OK);
        }
        else
        {
            check(CCC_bitset_push_back(&source, CCC_FALSE), CCC_RESULT_OK);
        }
    }
    CCC_Bitset destination = CCC_bitset_with_capacity(stack_allocator_allocate,
                                                      &allocator, 512, 0);
    CCC_Result r
        = CCC_bitset_copy(&destination, &source, stack_allocator_allocate);
    check(r, CCC_RESULT_OK);
    check(CCC_bitset_popcount(&source).count,
          CCC_bitset_popcount(&destination).count);
    check(CCC_bitset_count(&source).count,
          CCC_bitset_count(&destination).count);
    while (!CCC_bitset_is_empty(&source) && !CCC_bitset_is_empty(&destination))
    {
        CCC_Tribool const source_msb = CCC_bitset_pop_back(&source);
        CCC_Tribool const destination_msb = CCC_bitset_pop_back(&destination);
        if (CCC_bitset_count(&source).count % 2)
        {
            check(source_msb, CCC_TRUE);
            check(source_msb, destination_msb);
        }
        else
        {
            check(source_msb, CCC_FALSE);
            check(source_msb, destination_msb);
        }
    }
    check(CCC_bitset_is_empty(&source), CCC_bitset_is_empty(&destination));
    check_end();
}

check_static_begin(bitset_test_init_from)
{
    SV_String_view input = SV("110110");
    struct Stack_allocator allocator
        = stack_allocator_initialize(Bitblocks, to_blocks(32));
    CCC_Bitset b = CCC_bitset_from(stack_allocator_allocate, &allocator, 0,
                                   SV_len(input), '1', SV_begin(input));
    check(CCC_bitset_count(&b).count, SV_len(input));
    check(CCC_bitset_capacity(&b).count, SV_len(input));
    check(CCC_bitset_popcount(&b).count, 4);
    check(CCC_bitset_test(&b, 0), CCC_TRUE);
    check(CCC_bitset_test(&b, SV_len(input) - 1), CCC_FALSE);
    check_end();
}

check_static_begin(bitset_test_init_from_cap)
{
    SV_String_view input = SV("110110");
    struct Stack_allocator allocator
        = stack_allocator_initialize(Bitblocks, to_blocks(32));
    CCC_Bitset b = CCC_bitset_from(stack_allocator_allocate, &allocator, 0,
                                   SV_len(input), '1', SV_begin(input),
                                   SV_len(input) * 2);
    check(CCC_bitset_count(&b).count, SV_len(input));
    check(CCC_bitset_capacity(&b).count, (SV_len(input)) * 2);
    check(CCC_bitset_popcount(&b).count, 4);
    check(CCC_bitset_test(&b, 0), CCC_TRUE);
    check(CCC_bitset_test(&b, SV_len(input) - 1), CCC_FALSE);
    check(CCC_TRIBOOL_ERROR, CCC_bitset_test(&b, SV_len(input)));
    check(CCC_bitset_push_back(&b, CCC_TRUE), CCC_RESULT_OK);
    check(CCC_TRUE, CCC_bitset_test(&b, SV_len(input)));
    check(CCC_bitset_capacity(&b).count, (SV_len(input)) * 2);
    check_end();
}

check_static_begin(bitset_test_init_from_fail)
{
    SV_String_view input = SV("110110");
    /* Forgot allocation function. */
    CCC_Bitset b
        = CCC_bitset_from(NULL, NULL, 0, SV_len(input), '1', SV_begin(input));
    check(CCC_bitset_count(&b).count, 0);
    check(CCC_bitset_capacity(&b).count, 0);
    check(CCC_bitset_popcount(&b).count, 0);
    check(CCC_TRIBOOL_ERROR, CCC_bitset_test(&b, 0));
    check(CCC_TRIBOOL_ERROR, CCC_bitset_test(&b, 99));
    check_end(CCC_bitset_clear_and_free(&b););
}

check_static_begin(bitset_test_init_from_cap_fail)
{
    SV_String_view input = SV("110110");
    /* Forgot allocation function. */
    CCC_Bitset b = CCC_bitset_from(NULL, NULL, 0, SV_len(input), '1',
                                   SV_begin(input), 99);
    check(CCC_bitset_count(&b).count, 0);
    check(CCC_bitset_capacity(&b).count, 0);
    check(CCC_bitset_popcount(&b).count, 0);
    check(CCC_TRIBOOL_ERROR, CCC_bitset_test(&b, 0));
    check(CCC_TRIBOOL_ERROR, CCC_bitset_test(&b, 99));
    check_end(CCC_bitset_clear_and_free(&b););
}

check_static_begin(bitset_test_init_with_capacity)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(Bitblocks, to_blocks(10));
    CCC_Bitset b
        = CCC_bitset_with_capacity(stack_allocator_allocate, &allocator, 10);
    check(CCC_bitset_popcount(&b).count, 0);
    check(CCC_bitset_set(&b, 0, CCC_TRUE), CCC_FALSE);
    check(CCC_bitset_set(&b, 9, CCC_TRUE), CCC_FALSE);
    check(CCC_bitset_test(&b, 0), CCC_TRUE);
    check(CCC_bitset_test(&b, 9), CCC_TRUE);
    check_end();
}

check_static_begin(bitset_test_init_with_capacity_fail)
{
    CCC_Bitset b = CCC_bitset_with_capacity(NULL, NULL, 10);
    check(CCC_bitset_popcount(&b).count, 0);
    check(CCC_TRIBOOL_ERROR, CCC_bitset_set(&b, 0, CCC_TRUE));
    check(CCC_TRIBOOL_ERROR, CCC_bitset_set(&b, 9, CCC_TRUE));
    check(CCC_TRIBOOL_ERROR, CCC_bitset_test(&b, 0));
    check(CCC_TRIBOOL_ERROR, CCC_bitset_test(&b, 9));
    check_end(CCC_bitset_clear_and_free(&b););
}

int
main(void)
{
    return check_run(
        bitset_test_construct(), bitset_test_construct_with_literal(),
        bitset_test_copy_no_allocate(), bitset_test_copy_allocate(),
        bitset_test_init_from(), bitset_test_init_from_cap(),
        bitset_test_init_from_fail(), bitset_test_init_from_cap_fail(),
        bitset_test_init_with_capacity(),
        bitset_test_init_with_capacity_fail());
}
