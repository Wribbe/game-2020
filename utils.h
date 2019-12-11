#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <stdint.h>

#include <X11/Xlib.h>

struct xxWindow_native {
  Display * display;
  Window window;
  XEvent event;
  int screen;
};

struct xxWindow {
  const char * name;
  uint32_t width;
  uint32_t height;
  bool open;
  struct xxWindow_native native;
};

void
xxWindow_native_create(struct xxWindow * window)
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

enum XX_KEY {
  XX_ESC,
  XX_NUM_KEYS,
};

bool xx_state_keys_releases[XX_NUM_KEYS];

struct xxWindow
xxwindow_get(const char * name, uint32_t width, uint32_t height)
{
  struct xxWindow window = {name, width, height, true};
  xxWindow_native_create(&window);
  return window;
}

void
xxwindow_terminate(struct xxWindow * window)
{
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
xxinput_refresh(void)
{
}

bool
xxinput_released(enum XX_KEY key)
{
  return xx_state_keys_releases[key];
}
