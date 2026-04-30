cmake_minimum_required(VERSION 3.14)
project(test_libbattleship LANGUAGES C)

add_executable(test_libbattleship
    test_libbattleship.c
    ../lib/libbattleship.c
)

target_include_directories(test_libbattleship PRIVATE ../lib)
