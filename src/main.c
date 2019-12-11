#include "utils.h"

int
main(void)
{
  struct xxWindow window = xxwindow_get("name", 600, 800);
  while (xxwindow_isopen(&window)) {
  }
  xxwindow_terminate(&window);
}
