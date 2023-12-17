#pragma once

#include <cstdio>
#include <string>

#include <google/rpc/status.pb.h>

// Forward declarations
namespace grpcxx {
class context;
}

namespace svc {
template <class Impl> class Wrapper {
public:
	Wrapper() : _impl(), _service(*this) {}

	template <typename T>
	typename T::result_type call(grpcxx::context &ctx, const typename T::request_type &req) {
		typename T::result_type result;
		try {
			result = _impl.template call<T>(ctx, req);
		} catch (...) {
			google::rpc::Status s = _impl.exception();
			std::printf("Error: %s\n", s.message().c_str());

			std::string data;
			s.SerializeToString(&data);

			result = {
				{static_cast<grpcxx::status::code_t>(s.code()), base64Encode(data)}, std::nullopt};
		}

		return result;
	}

	constexpr auto &service() noexcept { return _service; }

private:
	static std::string base64Encode(const std::string &in) {
		static const char table[] = {
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};

		std::string out;
		out.reserve(4 * ((in.size() + 2) / 3)); // Encoding 3 bytes will result in 4 bytes

		int i = 0, j = -6;
		for (char c : in) {
			i  = (i << 8) + c;
			j += 8;

			while (j >= 0) {
				out.push_back(table[(i >> j) & 0x3f]);
				j -= 6;
			}
		}

		if (j > -6) {
			out.push_back(table[((i << 8) >> (j + 8)) & 0x3f]);
		}

		while (out.size() % 4) {
			out.push_back('=');
		}

		return out;
	}

	Impl               _impl;
	Impl::service_type _service;
};
} // namespace svc
