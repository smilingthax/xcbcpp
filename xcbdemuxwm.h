#pragma once

#include "xcbdemux.h"

namespace detail {

template <typename Parent, typename OnEmptyFn = void>
struct fltsig_wmdelete : private fltsig_base<void(xcb_client_message_event_t *), OnEmptyFn> {
  using base_t = fltsig_base<void(xcb_client_message_event_t *), OnEmptyFn>;

  fltsig_wmdelete(Parent &parent, xcb_atom_t wmprotocols_atom, xcb_atom_t wmdelete_atom)
    : parent(parent), wmprotocols_atom(wmprotocols_atom), wmdelete_atom(wmdelete_atom)
  { }

  void operator()(xcb_client_message_event_t *ev) {
    if (ev->type == wmprotocols_atom &&
        ev->data.data32[0] == wmdelete_atom) {
      base_t::signal.emit(ev);
    }
  }

  template <typename Fn = void (*)(xcb_client_message_event_t *)>
  Connection on(Fn&& fn, SignalFlags flags = {}) {
    // assert(signal.empty() == !conn.connected());
    if (!base_t::conn.connected()) {
      // NOTE: no flags for on_client_message, must be the same for all calls!
      base_t::conn = parent.on_client_message(*this);
    }
    return base_t::signal.connect((Fn&&)fn, flags);
  }

private:
  Parent &parent;
  xcb_atom_t wmprotocols_atom;
  xcb_atom_t wmdelete_atom;
};

} // namespace detail

struct XcbDemuxWithWM : XcbDemux {
private:
  struct connect_on_wm_delete {
    connect_on_wm_delete(XcbDemuxWithWM &parent) : parent(parent) {}

    template <typename Fn>
    Connection operator()(Fn&& fn) {
      return parent.on_wm_delete(fn);
    }

    XcbDemuxWithWM &parent;
  };

public:

  XcbDemuxWithWM(xcb_atom_t wmprotocols_atom, xcb_atom_t wmdelete_atom)
    : wmdelete_signal(*this, wmprotocols_atom, wmdelete_atom)
  { }

  template <typename Fn = void (*)(xcb_client_message_event_t *)>
  Connection on_wm_delete(Fn&& fn, SignalFlags flags = {}) {
    return wmdelete_signal.on(fn, flags);
  }

  template <typename Fn = void (*)(xcb_client_message_event_t *)>
  Connection on_wm_delete(xcb_window_t win, Fn&& fn, SignalFlags flags = {}) {
    return _connect_mem<
      xcb_client_message_event_t,
      xcb_window_t, &xcb_client_message_event_t::window,
      connect_on_wm_delete>(XCB_CLIENT_MESSAGE, win, (Fn&&)fn, flags, *this);
  }

protected:
  detail::fltsig_wmdelete<XcbDemuxWithWM> wmdelete_signal;
};

