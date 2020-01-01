/**
 * File:   mouse_thread_test.c
 * Author: AWTK Develop Team
 * Brief:  test mouse thread
 *
 * Copyright (c) 2018 - 2020  Guangzhou ZHIYUAN Electronics Co.,Ltd.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * License file for more details.
 *
 */

/**
 * History:
 * ================================================================
 * 2018-09-07 Li XianJing <xianjimli@hotmail.com> created
 *
 */

#include "mouse_thread.h"

int main(int argc, char* argv[]) {
  tk_thread_t* thread = NULL;
  if (argc < 2) {
    printf("%s filename\n", argv[0]);
    return 0;
  }

  thread = mouse_thread_run(argv[1], input_dispatch_print, NULL, 320, 480);

  tk_thread_join(thread);

  return 0;
}
