cmake_minimum_required(VERSION 3.15)
project(lispy)


aux_source_directory(. SRC_DIR)

add_executable(lispy ${SRC_DIR})

target_link_libraries(lispy edit)