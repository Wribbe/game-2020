#include "utils.h"

int
main(void)
{
//  size_t size_data = 0;
//  char * data = read_file("FreeSans.otf", &size_data);
//
//  if (data == NULL) {
//    return EXIT_FAILURE;
//  }
//
//  //for (int i=0; i < size_data; i++) {
//  for (int i=0; i < 100; i++) {
//    printf("%c", data[i]);
//  }

  struct xxWindow * window = xxwindow_get("name", 600, 800);

  while (xxwindow_open_get(window)) {
    xxinput_flush(window);
    if (xxinput_released(window, XX_KEY_ESC)) {
      printf("Escape was pressed!\n");
      xxwindow_open_set(window, false);
    }
  }
  xxwindow_terminate(window);
}
