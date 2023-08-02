#pragma once

#include <memory>
#include <mutex>

#include <hiredis.h>

#include "err/errors.h"

#include "config.h"

namespace datastore {
namespace redis {
using context_t = std::shared_ptr<redisContext>;
using reply_t   = std::shared_ptr<redisReply>;

class connection {
public:
	using lock_t = std::unique_lock<std::timed_mutex>;

	connection(const context_t &ptr, lock_t &&lock) noexcept : _ctx(ptr), _lock(std::move(lock)) {}

	template <typename... Args> reply_t cmd(const std::string_view str, Args &&...args) {
		reply_t reply(
			static_cast<reply_t::element_type *>(redisCommand(ctx(), str.data(), args...)),
			freeReplyObject);

		if (!reply || reply->type == REDIS_REPLY_ERROR) {
			throw err::DatastoreRedisCommandError();
		}

		return reply;
	}

private:
	context_t::element_type *ctx() const;

	context_t _ctx;
	lock_t    _lock;
};

connection conn();

template <typename... Args> inline auto cmd(const std::string_view str, Args &&...args) {
	return conn().cmd(str, args...);
}

void init(const config::redis_t &c);
} // namespace redis
} // namespace datastore
