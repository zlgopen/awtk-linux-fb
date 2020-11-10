/**
 * File:   input_dispatcher.
 * Author: AWTK Develop Team
 * Brief:  interface to dispatch input events
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

#include "input_dispatcher.h"

ret_t input_dispatch_print(void* ctx, const event_queue_req_t* e, const char* msg) {
  switch (e->event.type) {
    case EVT_POINTER_DOWN: {
      printf("%s pointer down:%d %d\n", msg, e->pointer_event.x, e->pointer_event.y);
      break;
    }
    case EVT_POINTER_UP: {
      printf("%s pointer up:%d %d\n", msg, e->pointer_event.x, e->pointer_event.y);
      break;
    }
    case EVT_POINTER_MOVE: {
      printf("%s pointer move:%d %d\n", msg, e->pointer_event.x, e->pointer_event.y);
      break;
    }
    case EVT_KEY_DOWN: {
      printf("%s keydown:%d\n", msg, e->key_event.key);
      break;
    }
    case EVT_KEY_UP: {
      printf("%s keyup:%d\n", msg, e->key_event.key);
      break;
    }
  }

  return RET_OK;
}
