cmake_minimum_required(VERSION 3.20)

project(kcli_demo_executable LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT TARGET kcli::sdk)
    find_package(KcliSDK CONFIG REQUIRED)
endif()
if(NOT TARGET alpha::sdk)
    find_package(AlphaSDK CONFIG REQUIRED)
endif()
if(NOT TARGET beta::sdk)
    find_package(BetaSDK CONFIG REQUIRED)
endif()
if(NOT TARGET gamma::sdk)
    find_package(GammaSDK CONFIG REQUIRED)
endif()

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

include(CTest)
if(BUILD_TESTING)
    set(KCLI_DEMO_CLI_CASES_SCRIPT
        ${CMAKE_CURRENT_SOURCE_DIR}/../../tests/demo_cli_cases.sh
    )

    add_test(
        NAME demo_cli_unknown_alpha_option_errors
        COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
                $<TARGET_FILE:kcli_demo_executable>
                unknown_alpha_option
    )

    add_test(
        NAME demo_cli_unknown_beta_option_errors
        COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
                $<TARGET_FILE:kcli_demo_executable>
                unknown_beta_option
    )

    add_test(
        NAME demo_cli_unknown_renamed_option_errors
        COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
                $<TARGET_FILE:kcli_demo_executable>
                unknown_renamed_option
    )

    add_test(
        NAME demo_cli_known_alpha_option_ok
        COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
                $<TARGET_FILE:kcli_demo_executable>
                known_alpha_option
    )
endif()
