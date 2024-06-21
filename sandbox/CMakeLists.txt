add_executable(hello_llvm)
target_sources(hello_llvm PRIVATE hello_llvm.cpp)
set_target_properties(hello_llvm PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

llvm_map_components_to_libnames(llvm_hello_llvm_required_libs core)
target_link_libraries(hello_llvm PRIVATE ${llvm_hello_llvm_required_libs})
target_include_directories(hello_llvm PRIVATE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(hello_llvm PRIVATE ${LLVM_DEFINITIONS_LIST})
