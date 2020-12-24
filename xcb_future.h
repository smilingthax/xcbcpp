#pragma once

#include "xcb_base.h"  // esp.: xcb_proto.tcc
#include <memory>

namespace detail {
struct wrap_unique_c_free {
  template <typename T>
  static auto map(T *t) -> std::unique_ptr<T, c_free_deleter> {
    return t;
  }
};
} // namespace detail

template <typename T, typename ReplyMapOp = detail::wrap_unique_c_free>
class XcbFuture {
  using Trait = detail::XcbRequestTraits<T>;
  using reply_t = decltype(ReplyMapOp::map((typename Trait::reply_t *)0));
public:
  template <typename... Args>
  XcbFuture(xcb_connection_t *conn, Args&&... args)
    : conn(conn) {
    cookie = Trait::request(conn, (Args&&)args...);
  }

  reply_t get() {
    xcb_generic_error_t *error = NULL;
    auto reply = Trait::get(conn, cookie, &error);
    if (reply) {
      return ReplyMapOp::map(reply);
    }
    // assert(error);
    const int code = error->error_code;
    free(error);
    throw XcbEventError(std::string("XcbFuture<") + Trait::name + "> error", code);
  }

private:
  xcb_connection_t *conn;
  typename Trait::cookie_t cookie;
};

