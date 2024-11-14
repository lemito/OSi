cmake_minimum_required(VERSION 3.27)
project(lab1)

add_executable(server server.c)
add_executable(client client.c)
# target_link_libraries(server client m)