cmake_minimum_required(VERSION 3.0.0)
project(doom VERSION 0.1.0)

add_compile_definitions(CUSTOM_OS NORMALUNIX)

find_library(SDL2_LIB SDL2)

add_subdirectory(linuxdoom-1.10)
add_subdirectory(port)
add_executable(doom main.c)

target_link_libraries(doom doom_lib ${SDL2_LIB})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
