#pragma once

#include <concepts>
#include <string>

namespace events {
template <typename T>
concept encodable = requires(T t) {
	t.encode();
};

template <typename T>
concept serializable = requires(T t) {
	{ t.serialize() } -> std::convertible_to<std::string>;
};
} // namespace events
