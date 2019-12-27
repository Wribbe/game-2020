#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <threads.h>
#include <sys/select.h>
#include <time.h>
#include <dlfcn.h>

#include <stdint.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <GL/gl.h>
#include <GL/glx.h>

#define BUFFER_EVENTS_SIZE 64

#define MSEC(t) t.tv_sec*1e3+t.tv_nsec/1e6

#define xxdlopen(name) dlopen(name, RTLD_LAZY)
#define xxdlsym(handle, name) dlsym(handle, name)
#define XXAPI

struct glx {
  void (* address_proc)(const GLubyte * name_proc);
  bool (* version)(Display * display, int * major, int * minor);
};

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
  XX_KEY_1,
  XX_KEY_2,
  XX_KEY_3,
  XX_KEY_4,
  XX_KEY_5,
  XX_KEY_6,
  XX_KEY_7,
  XX_KEY_8,
  XX_KEY_ESC,
};

#define XX_KEY_SIZE 256

enum XX_EVENT_TYPE {
  XX_EVENT_NONE,
  XX_EVENT_WINDOW_CLOSE,
  XX_EVENT_PRESS,
  XX_EVENT_RELEASE,
};

struct xxWindow_native {
  Display * display;
  Window window;
  XEvent event;
  int screen;
  struct glx glx;
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
  struct timespec time_last_flush;
  double state_keys_time[XX_KEY_SIZE];
  bool state_keys_released[XX_KEY_SIZE];
  bool state_keys_pressed[XX_KEY_SIZE];
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

    select(x11_fd+1, &in_fds, NULL, NULL, &tv);

    enum XX_EVENT_TYPE previous = XX_EVENT_NONE;
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
      switch(event_native->type) {
        case DestroyNotify:
          event->key = XX_KEY_NONE;
          event->type = XX_EVENT_WINDOW_CLOSE;
          window->index_event_first_empty++;
          break;
        case KeyPress:
        case KeyRelease:
          ; // Empty statement.
          XKeyEvent * event_cast = (XKeyEvent *)event_native;
          KeySym key_sym = XLookupKeysym(event_cast, 0);
          printf("event: %s\n", XKeysymToString(key_sym));
          if (event_cast->type == KeyPress && previous == XX_EVENT_RELEASE) {
            // Remove the previous release event and skip this one.
            window->index_event_first_empty--;
            continue;
          }
          event->key = event_cast->keycode;
          if (event_cast->type == KeyPress) {
            event->type = XX_EVENT_PRESS;
          } else {
            event->type = XX_EVENT_RELEASE;
          }
          previous = event->type;
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
xxwindow_get_native(struct xxWindow * window)
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
    ExposureMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask | \
    EnterWindowMask | LeaveWindowMask
  );
  XMapWindow(native->display, native->window);
  XFlush(native->display);
}

void
xxwindow_destroy_native(struct xxWindow * window)
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
  window->time_last_flush.tv_sec = 0;
  window->time_last_flush.tv_nsec = 0;

  mtx_init(&window->mutex, mtx_plain);
  cnd_init(&window->condition);
  xxwindow_get_native(window);
  mtx_lock(&window->mutex);
  thrd_create(&window->thread_input, handler_events, (void *)window);
  while (true) {
    if (cnd_wait(&window->condition, &window->mutex) == thrd_success) {
      break;
    }
  }
  mtx_unlock(&window->mutex);

  XXAPI void * h_glx = xxdlopen("libGL.so");
  struct glx glx = {
    .version = xxdlsym(h_glx, "glXQueryVersion"),
  };
  memcpy(&window->native.glx, &glx, sizeof(struct glx));
  int major=0, minor=0;
  window->native.glx.version(window->native.display, &major, &minor);
  printf("GLX version: %d,%d\n", major, minor);

  return window;
}

void
xxinput_flush(struct xxWindow * window)
{
  mtx_lock(&window->mutex);
  for (int i=0; i<XX_KEY_SIZE; i++) {
    window->state_keys_released[i] = false;
    window->state_keys_pressed[i] = false;
  }

  struct timespec current = {0};
  double msec_prev = MSEC(window->time_last_flush);
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &current);
  double msec_curr = MSEC(current);
  double msec_diff = msec_curr - msec_prev;
  window->time_last_flush.tv_sec = current.tv_sec;
  window->time_last_flush.tv_nsec = current.tv_nsec;

  // Add diff to all timers.
  for (int i=0; i<XX_KEY_SIZE; i++) {
    window->state_keys_time[i] += msec_diff;
  }
  for (int i=0; i<window->index_event_first_empty; i++) {
    struct xxEvent * event = &window->buffer_events[i];
    printf("  [%d]: ", i);
    switch (event->type) {
      case (XX_EVENT_PRESS):
        printf("Pressed %d.\n", event->key);
        switch (event->key) {
          case (XX_KEY_ESC):
            xxwindow_open_set_locked(window, false);
            break;
          default:
            break;
        }
        window->state_keys_pressed[event->key] = true;
        window->state_keys_time[event->key] = 0;
        break;
      case (XX_EVENT_RELEASE):
        printf("Released %d, held for %f ms.\n",
          event->key,
          window->state_keys_time[event->key]
        );
        window->state_keys_released[event->key] = true;
        window->state_keys_time[event->key] = 0;
        break;
      case (XX_EVENT_WINDOW_CLOSE):
        printf("Window Close.\n");
        xxwindow_open_set_locked(window, false);
        break;
      case (XX_EVENT_NONE):
        printf("None event should not be submitted.\n");
        break;
    }
  }
  window->index_event_first_empty = 0;
  mtx_unlock(&window->mutex);
}

void
xxwindow_destroy(struct xxWindow * window)
{
  if (xxwindow_open_get(window)) {
    xxwindow_open_set(window, false);
  }
  thrd_join(window->thread_input, NULL);
  mtx_destroy(&window->mutex);
  cnd_destroy(&window->condition);
  xxwindow_destroy_native(window);
  free(window);
}


bool
xxinput_released(struct xxWindow * window, enum XX_KEY key)
{
  return window->state_keys_released[key];
}
