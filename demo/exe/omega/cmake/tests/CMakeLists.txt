set(KCLI_DEMO_CLI_CASES_SCRIPT
    ${CMAKE_CURRENT_LIST_DIR}/demo_cli_cases.sh
)

add_test(
    NAME demo_cli_unknown_alpha_option_errors
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_omega>
            unknown_alpha_option
)

add_test(
    NAME demo_cli_unknown_beta_option_errors
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_omega>
            unknown_beta_option
)

add_test(
    NAME demo_cli_unknown_renamed_option_errors
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_omega>
            unknown_renamed_option
)

add_test(
    NAME demo_cli_known_alpha_option_ok
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_omega>
            known_alpha_option
)

add_test(
    NAME demo_cli_unknown_app_option_errors
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_omega>
            unknown_app_option
)

add_test(
    NAME demo_cli_known_and_unknown_option_errors
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_omega>
            known_and_unknown_option
)

add_test(
    NAME demo_cli_alpha_alias_option_ok
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_omega>
            alpha_alias_option
)

add_test(
    NAME demo_cli_positional_args_ok
    COMMAND ${KCLI_DEMO_CLI_CASES_SCRIPT}
            $<TARGET_FILE:kcli_demo_omega>
            positional_args
)
