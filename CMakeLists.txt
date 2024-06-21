cmake_minimum_required(VERSION 3.28)

project("LLVM Sandbox" LANGUAGES C CXX)

find_package(LLVM REQUIRED CONFIG)

separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})

add_subdirectory(sandbox)
