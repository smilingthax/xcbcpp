#pragma once

// This implementation uses per-handler type conversions.
// For non-rtti per-signal conversions (as per xcbevents.h), manual type ids (static {} id * ...) + comparison would also work.

#include "signals.h"
#include "xcb_base.h" // esp. for xcb_proto.tcc detail::event_handler_type
#include <unordered_map>
#include "getargtype.h"

struct XcbEventCallbacks {
  template <typename EventType, typename Fn = void (*)(EventType *)>
  Connection on(uint8_t type, Fn&& fn, SignalFlags flags = {}) {
    return _connect<EventType *>(type, (Fn&&)fn, flags);
  }

  template <uint8_t type,
            typename EventType = typename detail::event_handler_type<type>::type,
            typename Fn = void (*)(EventType *)>
  Connection on(Fn&& fn, SignalFlags flags = {}) {
    return _connect<EventType *>(type, (Fn&&)fn, flags);
  }

  template <typename Fn, typename EventTypePtr = get_arg_t<1, Fn>>
  Connection on(uint8_t type, Fn&& fn, SignalFlags flags = {}) {
    return _connect<EventTypePtr>(type, (Fn&&)fn, flags);
  }

  void emit(uint8_t type, xcb_generic_event_t *ev) {
    auto it = map.find(type);
    if (it == map.end()) {
      return;
    }
    it->second.emit(ev);
  }

  void emit(xcb_generic_event_t *ev) {
    emit(ev->response_type, ev);
  }

private:
  struct onempty final {
    onempty(XcbEventCallbacks &parent, uint8_t type)
      : parent(parent), type(type)
    { }

    void operator()() const {
      parent.map.erase(type);
    }

    XcbEventCallbacks &parent;
    uint8_t type;
  };

  template <typename EventTypePtr, typename Fn>
  Connection _connect(uint8_t type, Fn&& fn, SignalFlags flags) {
    auto res = map.emplace(type, onempty{*this, type});
    try {
      return res.first->second.connect([fn](xcb_generic_event_t *ev) {
        fn((EventTypePtr)ev);
      }, flags);
    } catch (...) {
      if (res.second) { // was inserted
        map.erase(type);
      }
      throw;
    }
  }

  std::unordered_map<uint8_t, Signal<void(xcb_generic_event_t *ev), onempty, detail::reduce_void>> map;
};

