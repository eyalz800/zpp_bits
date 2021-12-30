#pragma once
#include "gtest/gtest.h"
#include "zpp_bits.h"

inline std::string encode_hex(auto && view)
{
    auto to_hex = [](auto value) constexpr {
        if (value < 0xa) {
            return '0' + value;
        } else {
            return 'a' + (value - 0xa);
        }
    };

    std::string hex(view.size() * 2, '\0');
    for (auto i = 0; auto byte : view) {
        hex[i++] = to_hex((static_cast<std::uint8_t>(byte) >> 4) & 0xf);
        hex[i++] = to_hex(static_cast<std::uint8_t>(byte) & 0xf);
    }

    return hex;
}
