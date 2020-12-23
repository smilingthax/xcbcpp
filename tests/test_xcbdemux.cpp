#include "../xcbdemuxwm.h"

// g++ -Wall -std=c++11 -o test_xcbdemux test_xcbdemux.cpp `pkg-config --cflags xcb`

int main()
{
#if 0
  XcbEventCallbacks p;

  struct nothing { void operator()() const { printf("onempty\n"); } };

  detail::signal_for_mem<xcb_key_press_event_t, xcb_window_t, &xcb_key_press_event_t::event, nothing> k(XCB_KEY_PRESS, p);

  k.connect(34, [](xcb_key_press_event_t *ev) {
    printf("hi\n");
  }); // .disconnect();

  xcb_key_press_event_t kpe = { .event = 33};
  k(&kpe);
#else
//  XcbDemux dmux;
  XcbDemuxWithWM dmux{XCB_ATOM_NONE, XCB_ATOM_NONE};

  dmux.on_key_press(34, [](xcb_key_press_event_t *ev) {
    printf("hi\n");
  });

  dmux.on_key_press([](xcb_key_press_event_t *ev) {
    printf("hi all\n");
  });

  dmux.on_wm_delete([](xcb_client_message_event_t *ev) {
    printf("wmdelete\n");
  });

  dmux.on_wm_delete(12, [](xcb_client_message_event_t *ev) {
    printf("wmdelete win\n");
  });

  xcb_key_press_event_t kpe = { .response_type = XCB_KEY_PRESS, .event = 34};
  dmux.emit((xcb_generic_event_t*)&kpe);

//  xcb_client_message_event_t kcm = { .response_type = XCB_CLIENT_MESSAGE, .type = XCB_ATOM_NONE, .data = { .data32 = { XCB_ATOM_NONE + 0 } } };
  xcb_client_message_event_t kcm = { .response_type = XCB_CLIENT_MESSAGE, .window = 12, .type = XCB_ATOM_NONE, .data = { .data32 = { XCB_ATOM_NONE + 0 } } };
  dmux.emit((xcb_generic_event_t*)&kcm);
#endif

  return 0;
}

