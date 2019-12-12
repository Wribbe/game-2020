#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <threads.h>

#include <stdint.h>

#include <X11/Xlib.h>

#define BUFFER_EVENTS_SIZE 64

enum XX_KEY {
  XX_ESC,
  XX_NUM_KEYS,
};

enum XX_EVENT_TYPE {
  XX_EVENT_PRESS,
  XX_EVENT_RELEASE,
  XX_EVENT_HELD,
};

struct xxWindow_native {
  Display * display;
  Window window;
  XEvent event;
  int screen;
};

struct xxEvent {
  enum XX_EVENT_TYPE type;
  enum XX_KEY key;
};

struct xxWindow {
  const char * name;
  uint32_t width;
  uint32_t height;
  bool open;
  thrd_t thread_input;
  uint8_t index_event_first_empty;
  struct xxEvent buffer_events[BUFFER_EVENTS_SIZE];
  bool state_keys_released[XX_NUM_KEYS];
  bool state_keys_pressed[XX_NUM_KEYS];
  bool state_keys_held[XX_NUM_KEYS];
  struct xxWindow_native native;
};

void
xxwindow_native_create(struct xxWindow * window)
{
  struct xxWindow_native * native = &window->native;
  native->display = XOpenDisplay(NULL);
  if (native->display == NULL) {
    printf("Could not open display!\n");
  }
  native->screen = DefaultScreen(native->display);
  native->window = XCreateSimpleWindow(
    native->display,
    RootWindow(native->display, native->screen),
    10,
    10,
    window->height,
    window->width,
    1,
    BlackPixel(native->display, native->screen),
    WhitePixel(native->display, native->screen)
  );
  XSelectInput(native->display, native->window, ExposureMask | KeyPressMask);
  XMapWindow(native->display, native->window);

  XNextEvent(native->display, &native->event);
}

struct xxWindow
xxwindow_get(const char * name, uint32_t width, uint32_t height)
{
  struct xxWindow window = {name, width, height, true};
  xxwindow_native_create(&window);
  return window;
}

void
xxwindow_terminate(struct xxWindow * window)
{
  (void)window;
}

bool
xxwindow_open_get(struct xxWindow * window)
{
  return window->open;
}

void
xxwindow_open_set(struct xxWindow * window, bool state)
{
  window->open = state;
}

void
xxinput_flush(struct xxWindow * window)
{
  for (int i=0; i<XX_NUM_KEYS; i++) {
    window->state_keys_released[i] = false;
    window->state_keys_pressed[i] = false;
    window->state_keys_held[i] = false;
  }
  for (int i=0; i<window->index_event_first_empty; i++) {
    struct xxEvent * event = &window->buffer_events[i];
    switch (event->type) {
      case (XX_EVENT_PRESS):
        window->state_keys_pressed[event->key] = true;
        break;
      case (XX_EVENT_RELEASE):
        window->state_keys_released[event->key] = true;
        break;
      case (XX_EVENT_HELD):
        window->state_keys_held[event->key] = true;
        break;
    }
  }
}

bool
xxinput_released(struct xxWindow * window, enum XX_KEY key)
{
  return window->state_keys_released[key];
}
