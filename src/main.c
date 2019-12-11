#include "utils.h"

int
main(void)
{
  struct xxWindow window = xxwindow_get("name", 600, 800);
  while (xxwindow_open_get(&window)) {
    xxinput_refresh();
    if (xxinput_released(XX_ESC)) {
      xxwindow_open_set(&window, false);
    }
  }
  xxwindow_terminate(&window);
}
