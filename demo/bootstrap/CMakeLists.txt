cmake_minimum_required(VERSION 3.20)

project(kcli_demo_bootstrap LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT TARGET kcli::sdk)
    find_package(KcliSDK CONFIG REQUIRED)
endif()

add_executable(kcli_demo_bootstrap
    src/main.cpp
)

target_link_libraries(kcli_demo_bootstrap PRIVATE
    kcli::sdk
)

set_target_properties(kcli_demo_bootstrap PROPERTIES
    OUTPUT_NAME bootstrap
)
