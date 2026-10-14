cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 23)

project(argparse)

add_library(argparse STATIC argparse.cpp getopt.c)
target_compile_options(argparse PRIVATE "-O3")
