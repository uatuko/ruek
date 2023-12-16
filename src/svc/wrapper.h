#pragma once

#include <cstdio>

namespace svc {
template <class Impl> class Wrapper {
public:
	Wrapper() : _impl(), _service(*this) {}

	template <typename T> typename T::result_type call(const typename T::request_type &req) {
		typename T::result_type result;
		try {
			result = _impl.template call<T>(req);
		} catch (const std::exception &e) {
			std::printf("Error: %s\n", e.what());
			result = {_impl.exception(), std::nullopt};
		} catch (...) {
			std::printf("Error: Unknown\n");
			result = {_impl.exception(), std::nullopt};
		}

		return result;
	}

	constexpr auto &service() noexcept { return _service; }

private:
	Impl               _impl;
	Impl::service_type _service;
};
} // namespace svc
