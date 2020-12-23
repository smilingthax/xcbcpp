#include "../xcbevents.h"
//#include "../xcbevents-nortti.h"

// g++ -Wall -std=c++11 -o test_xcbevents test_xcbevents.cpp `pkg-config --cflags xcb`

void kp1(xcb_key_press_event_t *) { printf("kp1 key\n"); }
//void kp1(xcb_motion_notify_event_t *) { printf("kp1 motion\n"); }

int main()
{
  XcbEventCallbacks ecs;

/*
//  ecs.on<XCB_KEY_PRESS>([](auto *ev) {
  ecs.on<XCB_KEY_PRESS>([](xcb_key_press_event_t *ev) {
printf("hi %p\n", ev);
  });
//*/

/*
  ecs.on<xcb_key_press_event_t>(XCB_KEY_PRESS, [](xcb_key_press_event_t *ev) {
printf("ho %p\n", ev);
  });
//*/
  ecs.on(XCB_KEY_PRESS, [](xcb_key_press_event_t *ev) { // not type safe... !
printf("ho %p\n", ev);
  });

//  ecs.on<xcb_key_press_event_t>(XCB_KEY_PRESS, kp1);
  ecs.on(XCB_KEY_PRESS, kp1);
//  ecs.on<XCB_KEY_PRESS>(kp1);

xcb_key_press_event_t ev1 = { XCB_KEY_PRESS };
ecs.emit((xcb_generic_event_t*)&ev1);

xcb_motion_notify_event_t ev2 = { XCB_MOTION_NOTIFY };
ecs.emit((xcb_generic_event_t*)&ev2);

  return 0;
}

