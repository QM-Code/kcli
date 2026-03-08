set(KCLI_DEMO_CLI_CASES_SCRIPT
    ${CMAKE_CURRENT_LIST_DIR}/demo_cli_cases.sh
)

add_test(
    NAME demo_cli_unknown_alpha_option_errors
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_core>
            unknown_alpha_option
)

add_test(
    NAME demo_cli_known_alpha_option_ok
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_core>
            known_alpha_option
)

add_test(
    NAME demo_cli_output_option_ok
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_core>
            output_option
)
