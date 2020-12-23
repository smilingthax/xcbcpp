#include "xcb_base.h"
#include <string.h>

XcbError::XcbError(const std::string &str, int code)
  : std::runtime_error(str + " (" + std::to_string(code) + ")"), code(code)
{
}

XcbConnectionError::XcbConnectionError(int code)
  : XcbError(std::string("XcbConnectionError: ") + get_error_string(code), code)
{
}

const char *XcbConnectionError::get_error_string(int code)
{
  static constexpr const char *errstrs[] = {
    "Success",
    "I/O error",                      // XCB_CONN_ERROR
    "Unsupported extension",          // XCB_CONN_CLOSED_EXT_NOTSUPPORTED
    "Insufficient memory",            // XCB_CONN_CLOSED_MEM_INSUFFICIENT
    "Requested length exceeded",      // XCB_CONN_CLOSED_REQ_LEN_EXCEED
    "Could not parse DISPLAY string", // XCB_CONN_CLOSED_PARSE_ERR
    "No such screen on display",      // XCB_CONN_CLOSED_INVALID_SCREEN
    "Error during FD passing"         // XCB_CONN_CLOSED_FDPASSING_FAILED
  };

  if (code >= 0 && (unsigned int)code < sizeof(errstrs) / sizeof(*errstrs)) {
    return errstrs[code];
  }
  return ""; // TODO?
}

XcbEventError::XcbEventError(int code)
  : XcbError(std::string("XcbEventError: ") + get_error_string(code), code)
{
}

XcbEventError::XcbEventError(const std::string &prefix, int code)
  : XcbError(prefix + ": " + get_error_string(code), code)
{
}

const char *XcbEventError::get_error_string(int code)
{
  static constexpr const char *errstrs[] = {
    "Success", "BadRequest", "BadValue", "BadWindow", "BadPixmap",
    "BadAtom", "BadCursor", "BadFont", "BadMatch", "BadDrawable",
    "BadAccess", "BadAlloc", "BadColor", "BadGC", "BadIDChoice",
    "BadName", "BadLength", "BadImplementation"
  };

  if (code >= 0 && (unsigned int)code < sizeof(errstrs) / sizeof(*errstrs)) {
    return errstrs[code];
  }
  return ""; // TODO?
}

XcbConnection::XcbConnection(const char *name)
{
  conn = xcb_connect(name, &default_screen_num);
  owned = true;

  const int res = xcb_connection_has_error(conn);
  if (res) {
    throw new XcbConnectionError(res);
  }

  setup = xcb_get_setup(conn);
  cache_screens();
}

XcbConnection::XcbConnection(xcb_connection_t *conn, int default_screen_num)
  : conn(conn), owned(false), default_screen_num(default_screen_num)
{
  // assert(conn);   // TODO?
  // const int res = xcb_connection_has_error(conn);  // TDOO?
  setup = xcb_get_setup(conn);
  cache_screens();
}

XcbConnection::~XcbConnection()
{
  if (owned) {
    xcb_disconnect(conn);
  }
}

void XcbConnection::cache_screens()
{
  xcb_screen_iterator_t it = xcb_setup_roots_iterator(setup);

  screen_cache.clear();
  screen_cache.reserve(it.rem);
  for (; it.rem; xcb_screen_next(&it)) {
    screen_cache.push_back(it.data);
  }
}

xcb_screen_t *XcbConnection::screen_of_display(int screen_num)
{
  if (screen_num < 0 || (size_t)screen_num >= screen_cache.size()) {
    return NULL;
  }
  return screen_cache[screen_num];
}

void XcbConnection::flush()
{
  const int res = xcb_flush(conn);
  if (res <= 0) {
    const int code = xcb_connection_has_error(conn);
    if (res) {
      throw new XcbConnectionError(code);
    }
  } // else: success
}

XcbFuture<xcb_intern_atom_request_t, detail::intern_atom_atom> XcbConnection::intern_atom(const char *name, int len, bool create)
{
  // assert(name);
  if (len < 0) {
    len = strlen(name);
  }
  return {conn, !create, len, name};
}


XcbWindow::XcbWindow(
  XcbConnection &conn, xcb_window_t parent,
  uint16_t width, uint16_t height,
  uint32_t value_mask, std::initializer_list<const uintptr_t> value_list,
  uint16_t klass,
  int16_t x, int16_t y, uint16_t border_width,
  uint8_t depth, xcb_visualid_t visual)
  : conn(conn), win(conn.generate_id())
{
#if 1
  xcb_void_cookie_t ck = xcb_create_window_checked(
    conn,
    depth,
    win, parent,
    x, y, width, height, border_width,
    XCB_WINDOW_CLASS_INPUT_OUTPUT, visual,
    value_mask, value_list.begin());

  xcb_generic_error_t *error = xcb_request_check(conn, ck);
  if (error) {
    throw XcbEventError(error->error_code);
  }
#else
  xcb_create_window_checked(
    conn,
    depth,
    win, parent,
    x, y, width, height, border_width,
    XCB_WINDOW_CLASS_INPUT_OUTPUT, visual,
    value_mask, value_list);

  // conn.flush(); ?
#endif
}

XcbWindow::~XcbWindow()
{
#if 0
  xcb_void_cookie_t ck = xcb_destroy_window_checked(conn, win);

  xcb_generic_error_t *error = xcb_request_check(conn, ck);
  if (error) {
    fprintf(stderr, "xcb_destroy_window failed: %s (%d)\n",
      XcbEventError::get_error_string(error->error_code),
      error->error_code);
//    throw XcbEventError(error->error_code);   // dtor shall be nothrow
  }
#else
  xcb_destroy_window(conn, win);
  conn.flush();
#endif
}

void XcbWindow::map()
{
#if 1
  xcb_void_cookie_t ck = xcb_map_window_checked(conn, win);

  xcb_generic_error_t *error = xcb_request_check(conn, ck);
  if (error) {
    const int code = error->error_code;
    free(error);
    throw XcbEventError(code);
  }
#else
  xcb_map_window(conn, win);
  conn.flush();  // ?
#endif
}

void XcbWindow::unmap()
{
#if 1
  xcb_void_cookie_t ck = xcb_unmap_window_checked(conn, win);

  xcb_generic_error_t *error = xcb_request_check(conn, ck);
  if (error) {
    const int code = error->error_code;
    free(error);
    throw XcbEventError(code);
  }
#else
  xcb_unmap_window(conn, win);
  conn.flush();
#endif
}
