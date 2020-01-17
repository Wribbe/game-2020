#include "utils.h"

int
main(void)
{
  struct xxWindow * window = xxwindow_get("name", 600, 800);
  glXMakeCurrent(window->native.display, window->native.window, window->native.context);
  while (xxwindow_open_get(window)) {
    xxinput_flush(window);
    if (xxinput_released(window, XX_KEY_ESC)) {
      printf("Escape was pressed!\n");
      xxwindow_open_set(window, false);
    }
    //mtx_lock(&window->mutex);
    glClearColor(0, 0.5, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    window->native.glx.swap(window->native.display, window->native.window);
    //mtx_unlock(&window->mutex);
  }
  xxwindow_destroy(window);
}
