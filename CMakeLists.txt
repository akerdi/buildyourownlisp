cmake_minimum_required(VERSION 3.15)
project(sqlite01)


aux_source_directory(. SRC_DIR)

add_executable(sqlite01 ${SRC_DIR})

target_link_libraries(sqlite01 edit)