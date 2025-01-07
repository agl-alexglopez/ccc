#define TRAITS_USING_NAMESPACE_CCC
#include "ccc/traits.h"

int
main(void)
{
    ccc_buffer b = ccc_buf_init((int[5]){}, NULL, NULL, 5);
    if (is_empty(&b))
    {
        return 0;
    }
    return 1;
}
