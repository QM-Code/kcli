cmake_minimum_required(VERSION 3.20)

project(kcli_demo_executable LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(KcliSDK CONFIG REQUIRED)
find_package(AlphaSDK CONFIG REQUIRED)
find_package(BetaSDK CONFIG REQUIRED)
find_package(GammaSDK CONFIG REQUIRED)

add_executable(kcli_demo_executable
    src/main.cpp
)

target_link_libraries(kcli_demo_executable PRIVATE
    kcli::sdk
    alpha::sdk
    beta::sdk
    gamma::sdk
)

set_target_properties(kcli_demo_executable PROPERTIES
    OUTPUT_NAME test
)
