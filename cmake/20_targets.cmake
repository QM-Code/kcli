set(KCLI_SOURCES
    ${PROJECT_SOURCE_DIR}/src/kcli.cpp
    ${PROJECT_SOURCE_DIR}/src/kcli/impl.cpp
    ${PROJECT_SOURCE_DIR}/src/kcli/process.cpp
    ${PROJECT_SOURCE_DIR}/src/kcli/util.cpp
)

if(NOT KCLI_BUILD_STATIC AND NOT KCLI_BUILD_SHARED)
    message(FATAL_ERROR "kcli requires at least one of KCLI_BUILD_STATIC or KCLI_BUILD_SHARED to be ON.")
endif()

if(KCLI_BUILD_STATIC)
    add_library(kcli_sdk_static STATIC ${KCLI_SOURCES})
    add_library(kcli::sdk_static ALIAS kcli_sdk_static)

    target_include_directories(kcli_sdk_static
        PUBLIC
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )

    target_link_libraries(kcli_sdk_static PUBLIC spdlog::spdlog)

    set_target_properties(kcli_sdk_static PROPERTIES
        OUTPUT_NAME kcli
        EXPORT_NAME sdk_static
    )
endif()

if(KCLI_BUILD_SHARED)
    add_library(kcli_sdk_shared SHARED ${KCLI_SOURCES})
    add_library(kcli::sdk_shared ALIAS kcli_sdk_shared)

    target_include_directories(kcli_sdk_shared
        PUBLIC
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )

    target_link_libraries(kcli_sdk_shared PUBLIC spdlog::spdlog)

    set_target_properties(kcli_sdk_shared PROPERTIES
        OUTPUT_NAME kcli
        EXPORT_NAME sdk_shared
    )
endif()

if(TARGET kcli_sdk_shared)
    add_library(kcli::sdk ALIAS kcli_sdk_shared)
elseif(TARGET kcli_sdk_static)
    add_library(kcli::sdk ALIAS kcli_sdk_static)
endif()
