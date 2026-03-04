cmake_minimum_required(VERSION 3.20)

project(gamma VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT TARGET kcli::sdk)
    find_package(KcliSDK CONFIG REQUIRED)
endif()
include(CMakePackageConfigHelpers)

add_library(gamma_sdk STATIC
    src/cli.cpp
)
add_library(gamma::sdk ALIAS gamma_sdk)

target_include_directories(gamma_sdk
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(gamma_sdk PUBLIC kcli::sdk)

set_target_properties(gamma_sdk PROPERTIES
    OUTPUT_NAME gamma
    EXPORT_NAME sdk
)

install(TARGETS gamma_sdk
    EXPORT GammaSDKTargets
    ARCHIVE DESTINATION lib COMPONENT GammaSDK
    LIBRARY DESTINATION lib COMPONENT GammaSDK
    RUNTIME DESTINATION bin COMPONENT GammaSDK
    INCLUDES DESTINATION include
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION include
    COMPONENT GammaSDK
    FILES_MATCHING PATTERN "*.hpp"
)

install(EXPORT GammaSDKTargets
    FILE GammaSDKTargets.cmake
    NAMESPACE gamma::
    DESTINATION lib/cmake/GammaSDK
    COMPONENT GammaSDK
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GammaSDKConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/GammaSDKConfig.cmake
    INSTALL_DESTINATION lib/cmake/GammaSDK
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/GammaSDKConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/GammaSDKConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/GammaSDKConfigVersion.cmake
    DESTINATION lib/cmake/GammaSDK
    COMPONENT GammaSDK
)
