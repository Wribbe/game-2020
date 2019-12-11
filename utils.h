#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdint.h>

struct xxWindow {
  const char * name;
  uint32_t width;
  uint32_t height;
};

struct xxWindow
xxwindow_get(const char * name, uint32_t width, uint32_t height)
{
  return (struct xxWindow){name, width, height};
}

void
xxwindow_terminate(struct xxWindow * window)
{
}

int
xxwindow_isopen(struct xxWindow * window)
{
  return 1;
}
