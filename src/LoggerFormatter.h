#pragma once

#include <type_traits>
#include <fmt/format.h>
#include <tins/tins.h>

template <typename T>
struct fmt::formatter<
    T,
    std::enable_if_t<
        std::is_same_v<
            std::decay_t<decltype(std::declval<T>().to_string())>,
            std::string
        >,
        char
    >
> : fmt::formatter<std::string> {
    template <typename FormatCtx>
    auto format(const T &value, FormatCtx &ctx) {
        return fmt::formatter<std::string>::format(value.to_string(), ctx);
    }
};
