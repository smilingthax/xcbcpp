#include "../xcb_kbd.h"
#include <stdio.h>
#include <assert.h>
 #include <unistd.h>

// g++ -Wall -std=c++11 -o test_xcb_kbd test_xcb_kbd.cpp ../xcb_base.cpp ../xcb_kbd.cpp `pkg-config --cflags --libs xkbcommon-x11 xcb xcb-xkb`

// FIXME TODO: void XkbXcbProcessor::select_xkb_events() must be implemented!

#include "../xcb_base.h"

// #include "../xcb_xkb_improved.h"

int main()
{
  XcbConnection conn;

  XkbXcbProcessor kproc{conn};

  XcbWindow win(conn, conn.root_window(), 100, 100,
    XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK, {
      conn.white_pixel(),
      XCB_EVENT_MASK_KEY_PRESS
    });

  win.map();

  // TODO?! XErrorEvent can, with xkb extension, also be BadKeyboard (FIXME in run_once / XcbEventError ?!)
  while (conn.run_once([&kproc](xcb_generic_event_t *ev) {
if (kproc.process(ev)) return true;
    switch (ev->response_type & ~0x80) {
    case XCB_KEY_PRESS: {
      auto k = (xcb_key_press_event_t *)ev;
printf("0x%x 0x%x\n", k->detail, k->state);

      xkb_keysym_t keysym = kproc.get_one_sym(k->detail);
printf("ksym: %d\n", keysym);

      if (keysym == XKB_KEY_q) return false;

      break;
    }

    default:
      break;
    }
    return true;
  })) {
    usleep(100000);
  }

  return 0;
}
