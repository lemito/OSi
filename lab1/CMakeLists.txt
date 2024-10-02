cmake_minimum_required(VERSION 3.27)
project(lab1)

add_executable(posix_ipc-example-server posix_ipc-example-server.c)
add_executable(posix_ipc-example-client posix_ipc-example-client.c)
# target_link_libraries(server client m)