#include "../xcb_base.h"
#include <stdio.h>

 #include <unistd.h>  // usleep

// g++ -Wall -std=c++11 -o test_xcb_base test_xcb_base.cpp ../xcb_base.cpp `pkg-config --cflags --libs xcb`

int main()
{
  XcbConnection conn;
  XcbWindow win(conn, conn.root_window(), 100, 100,
    XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK, {
      conn.white_pixel(),
      /*XCB_EVENT_MASK_EXPOSURE |*/ XCB_EVENT_MASK_KEY_PRESS
    });

  auto wmdel_atoms = win.install_delete_handler();
  // FIXME? detect XCB_ATOM_NONE ...?

XcbFuture<xcb_intern_atom_request_t> fut(conn, true, 0, "");

  win.map();

  while (conn.run_once([wmdel_atoms, &win](xcb_generic_event_t *ev) {
printf("rt: %d\n", ev->response_type);
    switch (ev->response_type & ~0x80) {    // 0x80 is set, when SendEvent was used ("synthetic"??)
    case XCB_KEY_PRESS: {
      auto k = (xcb_key_press_event_t *)ev;
printf("0x%x 0x%x\n", k->detail, k->state);
if (k->detail == 0x18) return false; // 'q' ...
      break;
    }

    case XCB_CLIENT_MESSAGE: {
      auto cm = (xcb_client_message_event_t *)ev;
      if (cm->type == wmdel_atoms.first &&    // std::get<0>(wmdel_atoms)
          cm->data.data32[0] == wmdel_atoms.second) {
printf("0x%x 0x%x\n", cm->window, win.get_window());
        // TODO?! (cm->window == win) ?
        return false;
      }
    }
    }
    return true;
  })) {
    usleep(100000);
  }

  return 0;
}

