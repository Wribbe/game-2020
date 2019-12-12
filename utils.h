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
  mtx_t mutex;
  cnd_t condition;
  struct xxEvent buffer_events[BUFFER_EVENTS_SIZE];
  bool state_keys_released[XX_NUM_KEYS];
  bool state_keys_pressed[XX_NUM_KEYS];
  bool state_keys_held[XX_NUM_KEYS];
  struct xxWindow_native native;
};

int
handler_input(void * data)
{
  struct xxWindow * window = (struct xxWindow *)data;
  Display * display = window->native.display;
  XEvent * event = &window->native.event;
  mtx_lock(&window->mutex);
  printf("%s\n","INIT FROM THREAD");
  cnd_signal(&window->condition);
  printf("%s\n","SIGNAL DONE!");
  while (window->open) {
    XNextEvent(display, event);
    printf("%s\n","HELLO");
  }
  return 0;
}

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
}

void
xxwindow_native_destroy(struct xxWindow * window)
{
  XCloseDisplay(window->native.display);
}

struct xxWindow
xxwindow_get(const char * name, uint32_t width, uint32_t height)
{
  struct xxWindow window = {name, width, height, true};
  xxwindow_native_create(&window);
  mtx_init(&window.mutex, mtx_plain);
  cnd_init(&window.condition);
  thrd_create(&window.thread_input, handler_input, (void *)&window);
  mtx_lock(&window.mutex);
  while (true) {
    if (cnd_wait(&window.condition, &window.mutex) == thrd_success) {
      break;
    }
  }
  printf("Window done!\n");
  return window;
}

void
xxinput_flush(struct xxWindow * window)
{
  mtx_lock(&window->mutex);
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
  window->index_event_first_empty = 0;
  printf("UNLOCKING\n");
  mtx_unlock(&window->mutex);
  printf("DONE\n");
}

bool
xxwindow_open_get(struct xxWindow * window)
{
  return window->open;
}

void
xxwindow_open_set(struct xxWindow * window, bool state)
{
  mtx_lock(&window->mutex);
  window->open = state;
  mtx_unlock(&window->mutex);
}

void
xxwindow_terminate(struct xxWindow * window)
{
  if (xxwindow_open_get(window)) {
    xxwindow_open_set(window, false);
  }
  thrd_join(window->thread_input, NULL);
  mtx_destroy(&window->mutex);
  xxwindow_native_destroy(window);
}


bool
xxinput_released(struct xxWindow * window, enum XX_KEY key)
{
  return window->state_keys_released[key];
}
