cmake_minimum_required(VERSION 3.28)

project("ORC" LANGUAGES C CXX)

find_package(LLVM REQUIRED CONFIG)

add_executable(ORC_example)
set_target_properties(ORC_example PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)
target_sources(ORC_example PRIVATE orc_example.cpp)
llvm_map_components_to_libnames(llvm_orc_example_required_libs
    core support orcjit native X86 nativecodegen
)
target_link_libraries(ORC_example
    PRIVATE ${llvm_orc_example_required_libs}
)
