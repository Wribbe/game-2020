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


int
handler_input(void * data)
{
  struct xxWindow * window = (struct xxWindow *)data;
  mtx_lock(&window->mutex);
  cnd_signal(&window->condition);
  mtx_unlock(&window->mutex);
  while (window->open) {
    XNextEvent(window->native.display, &window->native.event);
    switch(window->native.event.type) {
      case(DestroyNotify):
        xxwindow_open_set(window, false);
        break;
      default:
        printf("Event type %d not matched.\n", window->native.event.type);
        break;
    }
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
  XSelectInput(
    native->display,
    native->window,
    ExposureMask | KeyPressMask | StructureNotifyMask
  );
  XMapWindow(native->display, native->window);
}

void
xxwindow_native_destroy(struct xxWindow * window)
{
  XCloseDisplay(window->native.display);
}

struct xxWindow *
xxwindow_get(const char * name, uint32_t width, uint32_t height)
{
  struct xxWindow * window = malloc(sizeof(struct xxWindow));

  window->name = name;
  window->width = width;
  window->height = height;
  window->open = true;
  window->index_event_first_empty = 0;

  mtx_init(&window->mutex, mtx_plain);
  cnd_init(&window->condition);
  xxwindow_native_create(window);
  mtx_lock(&window->mutex);
  thrd_create(&window->thread_input, handler_input, (void *)window);
  while (true) {
    if (cnd_wait(&window->condition, &window->mutex) == thrd_success) {
      break;
    }
  }
  mtx_unlock(&window->mutex);
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
  cnd_destroy(&window->condition);
  xxwindow_native_destroy(window);
  free(window);
}


bool
xxinput_released(struct xxWindow * window, enum XX_KEY key)
{
  return window->state_keys_released[key];
}
