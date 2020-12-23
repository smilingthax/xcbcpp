#pragma once

#include <memory>
#include <xcb/xproto.h>

namespace detail {

struct c_free_deleter {
  void operator()(void *table) const {
    ::free(table);
  }
};

template <typename T>
struct XcbRequestTraits;

#define MAKE_REQ_TRAIT(name) \
  template <>                                             \
  struct XcbRequestTraits<xcb_ ## name ## _request_t> {   \
    using cookie_t = xcb_ ## name ## _cookie_t;           \
    using reply_t = xcb_ ## name ## _reply_t;             \
    template <typename... Args>                           \
    static cookie_t request(xcb_connection_t *c, Args&&... args) { \
      return xcb_ ## name(c, (Args&&)args...);            \
    }                                                     \
    static reply_t *get(xcb_connection_t *conn, cookie_t cookie, xcb_generic_error_t **error) { \
      return xcb_ ## name ## _reply(conn, cookie, error); \
    }                                                     \
  }

MAKE_REQ_TRAIT(get_window_attributes);
MAKE_REQ_TRAIT(get_geometry);
MAKE_REQ_TRAIT(query_tree);
MAKE_REQ_TRAIT(intern_atom);
MAKE_REQ_TRAIT(get_atom_name);
MAKE_REQ_TRAIT(get_property);
MAKE_REQ_TRAIT(list_properties);
MAKE_REQ_TRAIT(get_selection_owner);
MAKE_REQ_TRAIT(grab_pointer);
MAKE_REQ_TRAIT(grab_keyboard);
MAKE_REQ_TRAIT(query_pointer);
MAKE_REQ_TRAIT(get_motion_events);
MAKE_REQ_TRAIT(translate_coordinates);
MAKE_REQ_TRAIT(get_input_focus);
MAKE_REQ_TRAIT(query_keymap);
MAKE_REQ_TRAIT(query_font);
MAKE_REQ_TRAIT(query_text_extents);
MAKE_REQ_TRAIT(list_fonts);
MAKE_REQ_TRAIT(list_fonts_with_info);
MAKE_REQ_TRAIT(get_font_path);
MAKE_REQ_TRAIT(get_image);
MAKE_REQ_TRAIT(list_installed_colormaps);
MAKE_REQ_TRAIT(alloc_color);
MAKE_REQ_TRAIT(alloc_named_color);
MAKE_REQ_TRAIT(alloc_color_cells);
MAKE_REQ_TRAIT(alloc_color_planes);
MAKE_REQ_TRAIT(query_colors);
MAKE_REQ_TRAIT(lookup_color);
MAKE_REQ_TRAIT(query_best_size);
MAKE_REQ_TRAIT(query_extension);
MAKE_REQ_TRAIT(list_extensions);
MAKE_REQ_TRAIT(get_keyboard_mapping);
MAKE_REQ_TRAIT(get_keyboard_control);
MAKE_REQ_TRAIT(get_pointer_control);
MAKE_REQ_TRAIT(get_screen_saver);
MAKE_REQ_TRAIT(list_hosts);
MAKE_REQ_TRAIT(set_pointer_mapping);
MAKE_REQ_TRAIT(get_pointer_mapping);
MAKE_REQ_TRAIT(set_modifier_mapping);
MAKE_REQ_TRAIT(get_modifier_mapping);

#undef MAKE_REQ_TRAIT

struct wrap_unique_c_free {
  template <typename T>
  static auto map(T *t) -> std::unique_ptr<T, c_free_deleter> {
    return t;
  }
};

struct intern_atom_atom {
  static xcb_atom_t map(xcb_intern_atom_reply_t *r) {
    return r->atom;
  }
};

template <uint8_t type>
struct event_handler_type;

#define SET_EVHT_ENTRY(val, name) \
  template <>                              \
  struct event_handler_type<XCB_ ## val> { \
    using type = xcb_ ## name ## _event_t; \
  }

SET_EVHT_ENTRY(KEY_PRESS, key_press);
SET_EVHT_ENTRY(KEY_RELEASE, key_release);
SET_EVHT_ENTRY(BUTTON_PRESS, button_press);
SET_EVHT_ENTRY(BUTTON_RELEASE, button_release);
SET_EVHT_ENTRY(MOTION_NOTIFY, motion_notify);
SET_EVHT_ENTRY(ENTER_NOTIFY, enter_notify);
SET_EVHT_ENTRY(LEAVE_NOTIFY, leave_notify);
SET_EVHT_ENTRY(FOCUS_IN, focus_in);
SET_EVHT_ENTRY(FOCUS_OUT, focus_out);
SET_EVHT_ENTRY(KEYMAP_NOTIFY, keymap_notify);
SET_EVHT_ENTRY(EXPOSE, expose);
SET_EVHT_ENTRY(GRAPHICS_EXPOSURE, graphics_exposure);
SET_EVHT_ENTRY(NO_EXPOSURE, no_exposure);
SET_EVHT_ENTRY(VISIBILITY_NOTIFY, visibility_notify);
SET_EVHT_ENTRY(CREATE_NOTIFY, create_notify);
SET_EVHT_ENTRY(DESTROY_NOTIFY, destroy_notify);
SET_EVHT_ENTRY(UNMAP_NOTIFY, unmap_notify);
SET_EVHT_ENTRY(MAP_NOTIFY, map_notify);
SET_EVHT_ENTRY(MAP_REQUEST, map_request);
SET_EVHT_ENTRY(REPARENT_NOTIFY, reparent_notify);
SET_EVHT_ENTRY(CONFIGURE_NOTIFY, configure_notify);
SET_EVHT_ENTRY(CONFIGURE_REQUEST, configure_request);
SET_EVHT_ENTRY(GRAVITY_NOTIFY, gravity_notify);
SET_EVHT_ENTRY(RESIZE_REQUEST, resize_request);
SET_EVHT_ENTRY(CIRCULATE_NOTIFY, circulate_notify);
SET_EVHT_ENTRY(CIRCULATE_REQUEST, circulate_request);
SET_EVHT_ENTRY(PROPERTY_NOTIFY, property_notify);
SET_EVHT_ENTRY(SELECTION_CLEAR, selection_clear);
SET_EVHT_ENTRY(SELECTION_REQUEST, selection_request);
SET_EVHT_ENTRY(SELECTION_NOTIFY, selection_notify);
SET_EVHT_ENTRY(COLORMAP_NOTIFY, colormap_notify);
SET_EVHT_ENTRY(CLIENT_MESSAGE, client_message);
SET_EVHT_ENTRY(MAPPING_NOTIFY, mapping_notify);
SET_EVHT_ENTRY(GE_GENERIC, ge_generic);

#undef SET_EVHT_ENTRY

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
    throw XcbEventError("xcb_intern_atom error", code); // TODO?!
  }

private:
  xcb_connection_t *conn;
  typename Trait::cookie_t cookie;
};

