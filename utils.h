#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <threads.h>
#include <sys/select.h>

#include <stdint.h>

#include <X11/Xlib.h>

#define BUFFER_EVENTS_SIZE 64

/*
#define mtx_lock_org mtx_lock
#define mtx_unlock_org mtx_unlock

#define mtx_lock(lock) \
  printf("Locking mutex in %s\n", __func__); \
  mtx_lock_org(lock); \
  printf("Locked mutex in %s\n", __func__);

#define mtx_unlock(unlock) \
  printf("Unlocking mutex in %s\n", __func__); \
  mtx_unlock_org(unlock); \
  printf("Unlocked mutex in %s\n", __func__);
*/

enum XX_KEY {
  XX_KEY_NONE,
  XX_KEY_ESC,
  XX_KEY_SIZE,
};

enum XX_EVENT_TYPE {
  XX_EVENT_WINDOW_CLOSE,
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
  bool state_keys_released[XX_KEY_SIZE];
  bool state_keys_pressed[XX_KEY_SIZE];
  bool state_keys_held[XX_KEY_SIZE];
  struct xxWindow_native native;
};

bool
xxwindow_open_get(struct xxWindow * window)
{
  return window->open;
}

void
xxwindow_open_set_locked(struct xxWindow * window, bool state)
{
  window->open = state;
}

void
xxwindow_open_set(struct xxWindow * window, bool state)
{
  mtx_lock(&window->mutex);
  xxwindow_open_set_locked(window, state);
  mtx_unlock(&window->mutex);
}

const char *
xxinput_event_to_string(XEvent * event)
{
  switch (event->type) {
    case (KeyPress):
      return "KeyPress";
      break;
    case (KeyRelease):
      return "KeyRelease";
      break;
    case (ButtonPress):
      return "ButtonPress";
      break;
    case (ButtonRelease):
      return "ButtonRelease";
      break;
    case (MotionNotify):
      return "MotionNotify";
      break;
    case (EnterNotify):
      return "EnterNotify";
      break;
    case (LeaveNotify):
      return "LeaveNotify";
      break;
    case (FocusIn):
      return "FocusIn";
      break;
    case (FocusOut):
      return "FocusOut";
      break;
    case (KeymapNotify):
      return "KeymapNotify";
      break;
    case (Expose):
      return "Expose";
      break;
    case (GraphicsExpose):
      return "GraphicsExpose";
      break;
    case (NoExpose):
      return "NoExpose";
      break;
    case (CirculateRequest):
      return "CirculateRequest";
      break;
    case (ConfigureRequest):
      return "ConfigureRequest";
      break;
    case (MapRequest):
      return "MapRequest";
      break;
    case (ResizeRequest):
      return "ResizeRequest";
      break;
    case (CirculateNotify):
      return "CirculateNotify";
      break;
    case (ConfigureNotify):
      return "ConfigureNotify";
      break;
    case (CreateNotify):
      return "CreateNotify";
      break;
    case (DestroyNotify):
      return "DestroyNotify";
      break;
    case (GravityNotify):
      return "GravityNotify";
      break;
    case (MapNotify):
      return "MapNotify";
      break;
    case (MappingNotify):
      return "MappingNotify";
      break;
    case (ReparentNotify):
      return "ReparentNotify";
      break;
    case (UnmapNotify):
      return "UnmapNotify";
      break;
    case (VisibilityNotify):
      return "VisibilityNotify";
      break;
    case (ColormapNotify):
      return "ColormapNotify";
      break;
    case (ClientMessage):
      return "ClientMessage";
      break;
    case (PropertyNotify):
      return "PropertyNotify";
      break;
    case (SelectionClear):
      return "SelectionClear";
      break;
    case (SelectionNotify):
      return "SelectionNotify";
      break;
    case (SelectionRequest):
      return "SelectionRequest";
      break;
    default:
      return "No mapping for event.";
  }
}


int
handler_events(void * data)
{
  struct xxWindow * window = (struct xxWindow *)data;

  mtx_lock(&window->mutex);
  cnd_signal(&window->condition);
  mtx_unlock(&window->mutex);

  int x11_fd = ConnectionNumber(window->native.display);
  int keycodes_min, keycodes_max;
  XDisplayKeycodes(window->native.display, &keycodes_min, &keycodes_max);
  printf("Keycodes: min: %d, max: %d\n", keycodes_min, keycodes_max);

  for (;;) {

    struct timeval tv = {0};
    tv.tv_sec=1;
    tv.tv_usec=0;
    fd_set in_fds;
    FD_ZERO(&in_fds);
    FD_SET(x11_fd, &in_fds);

    int num_ready_fds = select(x11_fd+1, &in_fds, NULL, NULL, &tv);
    if (num_ready_fds > 0) {
      printf("Event Received!\n");
    } else if (num_ready_fds == 0) {
      printf("Timer Fired!\n");
    }

    mtx_lock(&window->mutex);
    while(XPending(window->native.display)) {
      XNextEvent(window->native.display, &window->native.event);
      if (window->index_event_first_empty >= BUFFER_EVENTS_SIZE) {
        mtx_unlock(&window->mutex);
        break;
      }
      struct xxEvent * event = &window->buffer_events[
        window->index_event_first_empty
      ];
      XEvent * event_native = &window->native.event;
      printf("Event: %s\n", xxinput_event_to_string(event_native));
      switch(event_native->type) {
        case(DestroyNotify):
          event->key = XX_KEY_NONE;
          event->type = XX_EVENT_WINDOW_CLOSE;
          window->index_event_first_empty++;
          break;
        case(KeyPress):
        case(KeyRelease):
          ; // Empty statement.
          XKeyEvent * event_cast = (XKeyEvent *)event_native;
          event->type = event_cast->type == KeyPress ? XX_EVENT_PRESS : XX_EVENT_RELEASE;
          printf("Key event with code: %d\n", event_cast->keycode);
          window->index_event_first_empty++;
          break;
        default:
          printf("Event type %d not matched.\n", window->native.event.type);
          break;
      }
    }
    mtx_unlock(&window->mutex);
    if (!window->open) {
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
    ExposureMask | KeyPressMask | StructureNotifyMask | \
    EnterWindowMask | LeaveWindowMask
  );
  XMapWindow(native->display, native->window);
  XFlush(native->display);
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
  thrd_create(&window->thread_input, handler_events, (void *)window);
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
  for (int i=0; i<XX_KEY_SIZE; i++) {
    window->state_keys_released[i] = false;
    window->state_keys_pressed[i] = false;
    window->state_keys_held[i] = false;
  }
  for (int i=0; i<window->index_event_first_empty; i++) {
    printf("Processing event buffer index: %d\n", i);
    struct xxEvent * event = &window->buffer_events[i];
    switch (event->type) {
      case (XX_EVENT_PRESS):
        printf("Pressed.\n");
        window->state_keys_pressed[event->key] = true;
        break;
      case (XX_EVENT_RELEASE):
        printf("Released.\n");
        window->state_keys_released[event->key] = true;
        break;
      case (XX_EVENT_HELD):
        printf("Held.\n");
        window->state_keys_held[event->key] = true;
        break;
      case (XX_EVENT_WINDOW_CLOSE):
        printf("Window Close.\n");
        xxwindow_open_set_locked(window, false);
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
