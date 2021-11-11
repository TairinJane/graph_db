cmake_minimum_required(VERSION 3.20)
project(graph_lab C)

set(CMAKE_C_STANDARD 99)

add_executable(graph_lab main.c graph/graph.h storage/storage.h storage/storage.c storage/util.c storage/util.h storage/linked_list.c storage/linked_list.h)
