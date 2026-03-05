set(KCLI_SOURCES
    ${PROJECT_SOURCE_DIR}/src/kcli.cpp
    ${PROJECT_SOURCE_DIR}/src/kcli/impl.cpp
    ${PROJECT_SOURCE_DIR}/src/kcli/process.cpp
    ${PROJECT_SOURCE_DIR}/src/kcli/util.cpp
)

if(KCLI_BUILD_SHARED)
    set(_kcli_library_type SHARED)
else()
    set(_kcli_library_type STATIC)
endif()

add_library(kcli_sdk ${_kcli_library_type} ${KCLI_SOURCES})
add_library(kcli::sdk ALIAS kcli_sdk)

target_include_directories(kcli_sdk
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(kcli_sdk PUBLIC spdlog::spdlog)

set_target_properties(kcli_sdk PROPERTIES
    OUTPUT_NAME kcli
    EXPORT_NAME sdk
)
