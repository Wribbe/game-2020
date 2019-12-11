#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <stdint.h>

struct xxWindow {
  const char * name;
  uint32_t width;
  uint32_t height;
  bool open;
};

enum XX_KEY {
  XX_ESC,
  XX_NUM_KEYS,
};

bool xx_state_keys_releases[XX_NUM_KEYS];

struct xxWindow
xxwindow_get(const char * name, uint32_t width, uint32_t height)
{
  return (struct xxWindow){name, width, height, true};
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
