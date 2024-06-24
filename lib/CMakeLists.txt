add_library(attrib INTERFACE attrib.h)
add_library(random random.h random.c)
add_library(str_view str_view.h str_view.c)
add_library(cli cli.h cli.c)
target_link_libraries(cli PUBLIC
  str_view
)
add_library(queue queue.h queue.c)
target_link_libraries(queue attrib)
add_library(heap_pqueue heap_pqueue.h heap_pqueue.c)
target_link_libraries(heap_pqueue attrib)
add_library(pair_pqueue pair_pqueue.h pair_pqueue.c)
target_link_libraries(pair_pqueue attrib)
