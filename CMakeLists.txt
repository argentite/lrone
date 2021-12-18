cmake_minimum_required(VERSION 3.5)

project(lrone LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(lrone
    main.cpp
    grammar.cpp
    misc.cpp
    table.cpp
    parser.cpp
)

target_compile_options(lrone PRIVATE
    -Wall -Wextra -pedantic -Werror
)
