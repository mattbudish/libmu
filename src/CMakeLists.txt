project(mu)
add_library(mu STATIC mu.c)
target_link_libraries(mu microhttpd uv)
