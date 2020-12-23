#pragma once

#include <xcb/xcb.h>
#include <stdexcept>
#include <vector>

struct XcbError : std::runtime_error {
  XcbError(const std::string &str, int code);

  int code;
};

struct XcbConnectionError : XcbError {
  XcbConnectionError(int code);

  static const char *get_error_string(int code);
};


struct XcbEventError : XcbError {
  XcbEventError(int code);
  XcbEventError(const std::string &prefix, int code);

  static const char *get_error_string(int code);
};

#include "xcb_proto.tcc"

struct XcbConnection final {
  XcbConnection(const char *name = NULL);
  XcbConnection(xcb_connection_t *conn, int default_screen_num = 0);
  ~XcbConnection();

  uint32_t generate_id() {
    return xcb_generate_id(conn);
  }

  void flush();
  // ? void sync(); -> xcb_aux_sync(conn); ?

  int fd() { // for polling
    return xcb_get_file_descriptor(conn);
  }

  operator xcb_connection_t *() {
    return conn;
  }

  // or XCB_ATOM_NONE (w/ create = false)
  XcbFuture<xcb_intern_atom_request_t, detail::intern_atom_atom> intern_atom(const char *name, int len = -1, bool create = true);

  template <typename Fn>
  bool run_once(Fn&& fn) {
    xcb_generic_event_t *ev;
    while ((ev = xcb_poll_for_event(conn))) {
      if (ev->response_type == 0) {
        auto code = ((xcb_generic_error_t *)ev)->error_code;
        free(ev);
        throw XcbEventError(code);
      } else if (!fn(ev)) {
        free(ev);
        return false;
      }
      free(ev);
    }
    return true;
  }

//  const xcb_setup_t *get_setup() const { return setup; }

  int screen_count() {
    return screen_cache.size(); // or: return xcb_setup_roots_length(setup);
  }

  int default_screen() {
    return default_screen_num;
  }

  xcb_screen_t *screen_of_display(int screen_num);
//  xcb_screen_t *screen(int screen_num) { return screen(screen_num); }
  xcb_screen_t *screen() {
    return screen_of_display(default_screen_num);
  }

  xcb_window_t root_window(int screen_num) {
    const xcb_screen_t *screen = screen_of_display(screen_num);
    return (screen) ? screen->root : 0;
  }
  xcb_window_t root_window() {
    return root_window(default_screen_num);
  }

  xcb_visualid_t default_visual(int screen_num) {
    const xcb_screen_t *screen = screen_of_display(screen_num);
    return (screen) ? screen->root_visual : 0;
  }
  xcb_visualid_t default_visual() {
    return default_visual(default_screen_num);
  }
  // TODO?  xcb_visualtype_t *get(xcb_visualid_t vid)   ?

  // (default_gc, default_gc_of_screen(int screen_num) ?)

  uint32_t black_pixel(int screen_num) {
    const xcb_screen_t *screen = screen_of_display(screen_num);
    return (screen) ? screen->black_pixel : 0;
  }
  uint32_t black_pixel() {
    return black_pixel(default_screen_num);
  }

  uint32_t white_pixel(int screen_num) {
    const xcb_screen_t *screen = screen_of_display(screen_num);
    return (screen) ? screen->white_pixel : 0;
  }
  uint32_t white_pixel() {
    return white_pixel(default_screen_num);
  }

  uint32_t display_width(int screen_num) {
    const xcb_screen_t *screen = screen_of_display(screen_num);
    return (screen) ? screen->width_in_pixels : 0;
  }
  uint32_t display_width() {
    return display_width(default_screen_num);
  }

  uint32_t display_height(int screen_num) {
    const xcb_screen_t *screen = screen_of_display(screen_num);
    return (screen) ? screen->height_in_pixels : 0;
  }
  uint32_t display_height() {
    return display_height(default_screen_num);
  }

  uint32_t display_width_mm(int screen_num) {
    const xcb_screen_t *screen = screen_of_display(screen_num);
    return (screen) ? screen->width_in_millimeters : 0;
  }
  uint32_t display_width_mm() {
    return display_width_mm(default_screen_num);
  }

  uint32_t display_height_mm(int screen_num) {
    const xcb_screen_t *screen = screen_of_display(screen_num);
    return (screen) ? screen->height_in_millimeters : 0;
  }
  uint32_t display_height_mm() {
    return display_height_mm(default_screen_num);
  }

  // display_planes / default_depth ?
  // default_colormap ?
  // event_mask_of_screen ??

private:
  void cache_screens();

private:
  xcb_connection_t *conn;
  bool owned;
  int default_screen_num;

  const xcb_setup_t *setup;
  std::vector<xcb_screen_t *> screen_cache;
};

struct XcbWindow final {
  XcbWindow(
    XcbConnection &conn, xcb_window_t parent,
    uint16_t width, uint16_t height,
    uint32_t value_mask = 0, std::initializer_list<const uintptr_t> value_list = {},
    uint16_t klass = XCB_WINDOW_CLASS_INPUT_OUTPUT,
    int16_t x = 0, int16_t y = 0, uint16_t border_width = 0,
    uint8_t depth = XCB_COPY_FROM_PARENT,
    xcb_visualid_t visual = XCB_COPY_FROM_PARENT);
  ~XcbWindow();

  void map();
  void unmap();

  xcb_window_t get_window() { return win; }
private:
  XcbConnection &conn;
  xcb_window_t win;
};

