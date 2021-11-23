cmake_minimum_required(VERSION 3.20)
project(qbn C)

set(CMAKE_C_STANDARD 11)

add_library(
        qbn
        src/qbn.h
        src/qbn.c
        src/processing.h
        src/op.h
        src/op.macro
        src/util/process.h
        src/util/vector.h
        src/print.h src/limits.h src/util/std.h)

add_executable(
        test_qbn
        src/main.c
        src/qbn.h
        src/qbn.c
        src/processing.h
        src/op.macro
        src/op.h
        src/util/process.h
        src/util/vector.h
        src/print.h src/limits.h src/util/std.h)
