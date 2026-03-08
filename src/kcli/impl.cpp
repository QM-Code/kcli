#include "backend.hpp"

#include "util.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace kcli::backend {

void Reset(ParseState& state) {
    state.argc_ptr = nullptr;
    state.argv = nullptr;
    state.mode = Mode::EndUser;
    state.root_name.clear();
    state.policy = ParsePolicy{};
    state.commands.clear();
    state.command_order.clear();
}

void Initialize(ParseState& state, int& argc, char** argv) {
    state.argc_ptr = &argc;
    state.argv = argv;
    state.mode = Mode::EndUser;
    state.root_name.clear();
}

void Initialize(ParseState& state, int& argc, char** argv, std::string_view root) {
    state.argc_ptr = &argc;
    state.argv = argv;
    state.mode = Mode::Inline;
    state.root_name = detail::NormalizeRootNameOrThrow(root);
}

bool IsInlineMode(const ParseState& state) {
    return state.mode == Mode::Inline;
}

std::string GetRoot(const ParseState& state) {
    return state.root_name;
}

void SetPolicy(ParseState& state, const ParsePolicy& policy) {
    state.policy = policy;
}

void Implement(ParseState& state,
               std::string_view command,
               FlagHandler handler,
               std::string_view description) {
    if (!handler) {
        throw std::invalid_argument("kcli flag handler must not be empty");
    }

    const std::string command_id = detail::NormalizeCommandOrThrow(command);
    const std::string command_description = detail::NormalizeDescriptionOrThrow(description);

    const bool is_new = state.commands.find(command_id) == state.commands.end();
    CommandBinding binding{};
    binding.expects_value = false;
    binding.flag_handler = std::move(handler);
    binding.value_mode = ValueMode::None;
    binding.description = command_description;

    state.commands[command_id] = std::move(binding);
    if (is_new) {
        state.command_order.push_back(command_id);
    }
}

void Implement(ParseState& state,
               std::string_view command,
               ValueHandler handler,
               std::string_view description,
               ValueMode mode) {
    if (!handler) {
        throw std::invalid_argument("kcli value handler must not be empty");
    }

    const std::string command_id = detail::NormalizeCommandOrThrow(command);
    const std::string command_description = detail::NormalizeDescriptionOrThrow(description);

    const bool is_new = state.commands.find(command_id) == state.commands.end();
    CommandBinding binding{};
    binding.expects_value = true;
    binding.value_handler = std::move(handler);
    binding.value_mode = mode;
    binding.description = command_description;

    state.commands[command_id] = std::move(binding);
    if (is_new) {
        state.command_order.push_back(command_id);
    }
}

} // namespace kcli::backend
