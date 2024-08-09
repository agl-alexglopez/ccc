add_library(attrib INTERFACE attrib.h)
add_library(random random.h random.c)
add_library(cli cli.h cli.c)
target_link_libraries(cli PUBLIC
  str_view::str_view
)
add_library(queue queue.h queue.c)
target_link_libraries(queue attrib)
