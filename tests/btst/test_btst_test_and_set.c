#include "ccc/bitset.h"
#include "checkers.h"

CHECK_BEGIN_STATIC_FN(btst_test_set_one)
{
    ccc_bitset btst
        = ccc_btst_init((ccc_bitblock[ccc_bitblocks(10)]){}, 10, NULL, NULL);
    CHECK(ccc_btst_capacity(&btst), 10);
    /* Was false before. */
    CHECK(ccc_btst_set(&btst, 5, CCC_TRUE), CCC_FALSE);
    CHECK(ccc_btst_set_at(&btst, 5, CCC_TRUE), CCC_TRUE);
    CHECK(ccc_btst_popcount(&btst), 1);
    CHECK(ccc_btst_set(&btst, 5, CCC_FALSE), CCC_TRUE);
    CHECK(ccc_btst_set_at(&btst, 5, CCC_FALSE), CCC_FALSE);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(btst_test_set_shuffled)
{
    ccc_bitset btst
        = ccc_btst_init((ccc_bitblock[ccc_bitblocks(10)]){}, 10, NULL, NULL);
    size_t const larger_prime = 11;
    for (size_t i = 0, shuf_i = larger_prime % 10; i < 10;
         ++i, shuf_i = (shuf_i + larger_prime) % 10)
    {
        CHECK(ccc_btst_set(&btst, i, CCC_TRUE), CCC_FALSE);
        CHECK(ccc_btst_set_at(&btst, i, CCC_TRUE), CCC_TRUE);
    }
    CHECK(ccc_btst_popcount(&btst), 10);
    for (size_t i = 0; i < 10; ++i)
    {
        CHECK(ccc_btst_test(&btst, i), CCC_TRUE);
        CHECK(ccc_btst_test_at(&btst, i), CCC_TRUE);
    }
    CHECK(ccc_btst_capacity(&btst), 10);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(btst_test_set_all)
{
    ccc_bitset btst
        = ccc_btst_init((ccc_bitblock[ccc_bitblocks(10)]){}, 10, NULL, NULL);
    CHECK(ccc_btst_set_all(&btst, CCC_TRUE), CCC_OK);
    CHECK(ccc_btst_popcount(&btst), 10);
    for (size_t i = 0; i < 10; ++i)
    {
        CHECK(ccc_btst_test(&btst, i), CCC_TRUE);
        CHECK(ccc_btst_test_at(&btst, i), CCC_TRUE);
    }
    CHECK(ccc_btst_capacity(&btst), 10);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(btst_test_reset)
{
    ccc_bitset btst
        = ccc_btst_init((ccc_bitblock[ccc_bitblocks(10)]){}, 10, NULL, NULL);
    size_t const larger_prime = 11;
    for (size_t i = 0, shuf_i = larger_prime % 10; i < 10;
         ++i, shuf_i = (shuf_i + larger_prime) % 10)
    {
        CHECK(ccc_btst_set(&btst, i, CCC_TRUE), CCC_FALSE);
        CHECK(ccc_btst_set_at(&btst, i, CCC_TRUE), CCC_TRUE);
    }
    CHECK(ccc_btst_reset(&btst, 9), CCC_TRUE);
    CHECK(ccc_btst_reset(&btst, 9), CCC_FALSE);
    CHECK(ccc_btst_popcount(&btst), 9);
    CHECK(ccc_btst_capacity(&btst), 10);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(btst_test_reset_all)
{
    ccc_bitset btst
        = ccc_btst_init((ccc_bitblock[ccc_bitblocks(10)]){}, 10, NULL, NULL);
    CHECK(ccc_btst_capacity(&btst), 10);
    CHECK(ccc_btst_set_all(&btst, CCC_TRUE), CCC_OK);
    CHECK(ccc_btst_popcount(&btst), 10);
    CHECK(ccc_btst_reset_all(&btst), CCC_OK);
    CHECK(ccc_btst_popcount(&btst), 0);
    CHECK_END_FN();
}

/* Returns if the box is valid. 1 for valid, 0 for invalid, -1 for an error */
ccc_tribool
validate_box(int board[9][9], ccc_bitset *const row_check,
             ccc_bitset *const col_check, size_t const row_start,
             size_t const col_start)
{
    ccc_bitset box_check
        = ccc_btst_init((ccc_bitblock[ccc_bitblocks(9)]){}, 9, NULL, NULL);
    ccc_tribool was_on = CCC_FALSE;
    for (size_t r = row_start; r < row_start + 3; ++r)
    {
        for (size_t c = col_start; c < col_start + 3; ++c)
        {
            if (!board[r][c])
            {
                continue;
            }
            /* Need the zero based digit. */
            size_t const digit = board[r][c] - 1;
            was_on = ccc_btst_set(&box_check, digit, CCC_TRUE);
            if (was_on != CCC_FALSE)
            {
                goto done;
            }
            was_on = ccc_btst_set(row_check, (r * 9) + digit, CCC_TRUE);
            if (was_on != CCC_FALSE)
            {
                goto done;
            }
            was_on = ccc_btst_set(col_check, (c * 9) + digit, CCC_TRUE);
            if (was_on != CCC_FALSE)
            {
                goto done;
            }
        }
    }
done:
    switch (was_on)
    {
    case CCC_TRUE:
        return CCC_FALSE;
    case CCC_FALSE:
        return CCC_TRUE;
    case CCC_BOOL_ERR:
        return CCC_BOOL_ERR;
    }
}

/* A small problem like this is a perfect use case for a stack based bit set.
   All sizes are known at compile time meaning we get memory management for
   free and the optimal space and time complexity for this problem. */

CHECK_BEGIN_STATIC_FN(btst_test_valid_sudoku)
{
    /* clang-format off */
    int valid_board[9][9] =
    {{5,3,0,0,7,0,0,0,0}
    ,{6,0,0,1,9,5,0,0,0}
    ,{0,9,8,0,0,0,0,6,0}
    ,{8,0,0,0,6,0,0,0,3}
    ,{4,0,0,8,0,3,0,0,1}
    ,{7,0,0,0,2,0,0,0,6}
    ,{0,6,0,0,0,0,2,8,0}
    ,{0,0,0,4,1,9,0,0,5}
    ,{0,0,0,0,8,0,0,7,9}};
    /* clang-format on */
    ccc_bitset row_check = ccc_btst_init(
        (ccc_bitblock[ccc_bitblocks(9ULL * 9ULL)]){}, 9ULL * 9ULL, NULL, NULL);
    ccc_bitset col_check = ccc_btst_init(
        (ccc_bitblock[ccc_bitblocks(9ULL * 9ULL)]){}, 9ULL * 9ULL, NULL, NULL);
    size_t const box_step = 3;
    for (size_t row = 0; row < 9ULL; row += box_step)
    {
        for (size_t col = 0; col < 9ULL; col += box_step)
        {
            ccc_tribool const valid
                = validate_box(valid_board, &row_check, &col_check, row, col);
            CHECK(valid, CCC_TRUE);
        }
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(btst_test_invalid_sudoku)
{
    /* clang-format off */
    int invalid_board[9][9] =
    {{8,3,0,0,7,0,0,0,0} /* 8 in first box top left. */
    ,{6,0,0,1,9,5,0,0,0}
    ,{0,9,8,0,0,0,0,6,0} /* 8 in first box bottom right. */
    ,{8,0,0,0,6,0,0,0,3} /* 8 also overlaps with 8 in top left by row. */
    ,{4,0,0,8,0,3,0,0,1}
    ,{7,0,0,0,2,0,0,0,6}
    ,{0,6,0,0,0,0,2,8,0}
    ,{0,0,0,4,1,9,0,0,5}
    ,{0,0,0,0,8,0,0,7,9}};
    /* clang-format on */
    ccc_bitset row_check = ccc_btst_init(
        (ccc_bitblock[ccc_bitblocks(9ULL * 9ULL)]){}, 9ULL * 9ULL, NULL, NULL);
    ccc_bitset col_check = ccc_btst_init(
        (ccc_bitblock[ccc_bitblocks(9ULL * 9ULL)]){}, 9ULL * 9ULL, NULL, NULL);
    size_t const box_step = 3;
    ccc_tribool pass = CCC_TRUE;
    for (size_t row = 0; row < 9ULL; row += box_step)
    {
        for (size_t col = 0; col < 9ULL; col += box_step)
        {
            pass
                = validate_box(invalid_board, &row_check, &col_check, row, col);
            CHECK(pass != CCC_BOOL_ERR, true);
            if (!pass)
            {
                goto done;
            }
        }
    }
done:
    CHECK(pass, CCC_FALSE);
    CHECK_END_FN();
}

int
main(void)
{
    return CHECK_RUN(btst_test_set_one(), btst_test_set_shuffled(),
                     btst_test_set_all(), btst_test_reset(),
                     btst_test_reset_all(), btst_test_valid_sudoku(),
                     btst_test_invalid_sudoku());
}
