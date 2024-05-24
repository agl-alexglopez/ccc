add_library(str_view str_view.h str_view.c)
add_library(cli cli.h cli.c)
target_link_libraries(cli PUBLIC
  str_view
)
