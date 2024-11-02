cmake_minimum_required(VERSION 3.27)
project(lab2 VERSION 1.0 LANGUAGES CXX)
# -fsanitize=address  -fsanitize=undefined -fno-sanitize-recover=all -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow -fno-sanitize=null -fno-sanitize=alignment 
# -
list(APPEND CMAKE_CXX_FLAGS "-lm -Wall -Werror -Wpedantic -Wextra -g -Wno-write-strings -fsanitize=thread ")

set(CMAKE_CXX_FLAGS_LABS  "-lm -Wall -Werror -Wpedantic -Wextra -g -fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow -fno-sanitize=null -fno-sanitize=alignment" FORCE)

add_executable(lab2 main.cpp)
add_executable(lab2_cpy main_copy.cpp)