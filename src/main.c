#include "utils.h"

int
main(void)
{
  struct xxWindow * window = xxwindow_get("name", 600, 800);
  while (xxwindow_open_get(window)) {
    xxinput_flush(window);
    if (xxinput_released(window, XX_KEY_ESC)) {
      printf("Escape was pressed!\n");
      xxwindow_open_set(window, false);
    }
  }
  xxwindow_destroy(window);
}
