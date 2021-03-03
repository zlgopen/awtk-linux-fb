/**
 * File:   mouse_thread.c
 * Author: AWTK Develop Team
 * Brief:  thread to read /dev/input/
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

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include "tkc/mem.h"
#include "base/keys.h"
#include "tkc/thread.h"
#include "mouse_thread.h"
#include "tkc/utils.h"

#ifndef EV_SYN
#define EV_SYN 0x00
#endif

typedef struct _run_info_t {
  int fd;
  int32_t x;
  int32_t y;
  int32_t max_x;
  int32_t max_y;
  void* dispatch_ctx;
  char* filename;
  input_dispatch_t dispatch;
  union {
    int8_t d[3];
    struct input_event e; /* for EasyARM-iMX280A_283A_287A */
  } data;
  bool_t left_pressed;
  bool_t rigth_pressed;
  bool_t middle_pressed;
  event_queue_req_t req;
} run_info_t;

static ret_t input_dispatch(run_info_t* info) {
  ret_t ret = info->dispatch(info->dispatch_ctx, &(info->req), "mouse");
  info->req.event.type = EVT_NONE;

  return ret;
}

static ret_t input_dispatch_set_mouse_event(run_info_t* info, event_queue_req_t* req, bool_t left, bool_t right, bool_t middle, bool_t normal) {
  if (normal) {
    if (info->left_pressed || info->rigth_pressed || info->middle_pressed) {
      if (info->left_pressed) {
        info->left_pressed = FALSE;
        req->event.type = EVT_POINTER_UP;
        req->pointer_event.pressed = FALSE;
      }
      if (info->rigth_pressed) {
        info->rigth_pressed = FALSE;
        req->event.type = EVT_CONTEXT_MENU;
      }
      if (info->middle_pressed) {
        info->middle_pressed = FALSE;
        req->event.type = EVT_KEY_UP;
        req->key_event.key = TK_KEY_WHEEL;
      }
    } else {
      req->event.type = EVT_POINTER_MOVE;
    }
  } else {
    if (left) {
      if (!info->left_pressed) {
        info->left_pressed = TRUE;
        req->pointer_event.pressed = TRUE;
        req->event.type = EVT_POINTER_DOWN;
      } else {
        req->event.type = EVT_POINTER_MOVE;
      }
    } else if (middle) {
      info->middle_pressed = TRUE;
      req->event.type = EVT_KEY_DOWN;
      req->key_event.key = TK_KEY_WHEEL;
    } else if (right) {      
      info->rigth_pressed = TRUE;
      req->event.type = EVT_POINTER_MOVE;
    } else {
      req->event.type = EVT_POINTER_MOVE;
    }
  }
  return RET_OK;
}

static ret_t input_dispatch_one_event(run_info_t* info) {
  int ret = 0;
  event_queue_req_t* req = &(info->req);

  if (info->fd < 0) {
    ret = -1;
  } else {
    ret = read(info->fd, &info->data.e, sizeof(info->data.e));
  }

  if (ret < 0) {
    sleep(2);

    if (access(info->filename, R_OK) == 0) {
      if (info->fd >= 0) {
        close(info->fd);
      }

      info->fd = open(info->filename, O_RDONLY);
      if (info->fd < 0) {
        log_debug("%s:%d: open mouse failed, fd=%d, filename=%s\n", __func__, __LINE__, info->fd,
                  info->filename);
        perror("print mouse: ");
      } else {
        log_debug("%s:%d: open mouse successful, fd=%d, filename=%s\n", __func__, __LINE__,
                  info->fd, info->filename);
      }
    }
  }

  if (ret == 3) {
    bool_t left = info->data.d[0] & 0x1;
    bool_t right = info->data.d[0] & 0x2;
    bool_t middle = info->data.d[0] & 0x4;
    bool_t normal = !(info->data.d[0] & 0x7);
    int x = info->data.d[1];
    int y = info->data.d[2];

    info->x += x;
    info->y -= y;

    if (info->x < 0) {
      info->x = 0;
    }
    if (info->x > info->max_x) {
      info->x = info->max_x;
    }
    if (info->y < 0) {
      info->y = 0;
    }
    if (info->y > info->max_y) {
      info->y = info->max_y;
    }

    req->pointer_event.x = info->x;
    req->pointer_event.y = info->y;

    input_dispatch_set_mouse_event(info, req, left, right, middle, normal);
    input_dispatch(info);
  } else if (ret == sizeof(info->data.e)) {
    switch (info->data.e.type) {
      case EV_KEY: {
        bool_t left = info->data.e.value && (info->data.e.code == BTN_LEFT || info->data.e.code == BTN_TOUCH);
        bool_t right = info->data.e.value && info->data.e.code == BTN_RIGHT;
        bool_t middle = info->data.e.value && info->data.e.code == BTN_MIDDLE;
        bool_t normal = !(left || right || middle);
        input_dispatch_set_mouse_event(info, req, left, right, middle, normal);
        break;
      }
      case EV_ABS: {
        switch (info->data.e.code) {
          case ABS_X: {
            req->pointer_event.x = info->data.e.value;
            break;
          }
          case ABS_Y: {
            req->pointer_event.y = info->data.e.value;
            break;
          }
          default:
            break;
        }

        if (req->event.type == EVT_NONE) {
          req->event.type = EVT_POINTER_MOVE;
        }

        break;
      }
      case EV_REL: {
        switch (info->data.e.code) {
          case REL_X: {
            req->pointer_event.x += info->data.e.value;

            if (req->pointer_event.x < 0) {
              req->pointer_event.x = 0;
            }
            if (req->pointer_event.x > info->max_x) {
              req->pointer_event.x = info->max_x;
            }
            break;
          }
          case REL_Y: {
            req->pointer_event.y += info->data.e.value;
            if (req->pointer_event.y < 0) {
              req->pointer_event.y = 0;
            }
            if (req->pointer_event.y > info->max_y) {
              req->pointer_event.y = info->max_y;
            }
            break;
          }
          default:
            break;
        }

        if (req->event.type == EVT_NONE) {
          req->event.type = EVT_POINTER_MOVE;
        }

        break;
      }
      case EV_SYN: {
        switch (req->event.type) {
          case EVT_KEY_UP:
          case EVT_KEY_DOWN:
          case EVT_CONTEXT_MENU:
          case EVT_POINTER_DOWN:
          case EVT_POINTER_MOVE:
          case EVT_POINTER_UP: {
            return input_dispatch(info);
          }
          default:
            break;
        }
        break;
      }
      default:
        break;
    }
  }

  return RET_OK;
}

void* input_run(void* ctx) {
  run_info_t info = *(run_info_t*)ctx;
  if (info.fd < 0) {
    log_debug("%s:%d: open mouse failed, fd=%d, filename=%s\n", __func__, __LINE__, info.fd,
              info.filename);
  } else {
    log_debug("%s:%d: open mouse successful, fd=%d, filename=%s\n", __func__, __LINE__, info.fd,
              info.filename);
  }

  TKMEM_FREE(ctx);
  while (input_dispatch_one_event(&info) == RET_OK)
    ;
  close(info.fd);
  TKMEM_FREE(info.filename);

  return NULL;
}

static run_info_t* info_dup(run_info_t* info) {
  run_info_t* new_info = TKMEM_ZALLOC(run_info_t);

  *new_info = *info;

  return new_info;
}

tk_thread_t* mouse_thread_run(const char* filename, input_dispatch_t dispatch, void* ctx,
                              int32_t max_x, int32_t max_y) {
  run_info_t info;
  tk_thread_t* thread = NULL;
  return_value_if_fail(filename != NULL && dispatch != NULL, NULL);

  memset(&info, 0x00, sizeof(info));

  info.max_x = max_x;
  info.max_y = max_y;
  info.dispatch_ctx = ctx;
  info.dispatch = dispatch;
  info.fd = open(filename, O_RDONLY);
  info.filename = tk_strdup(filename);

  thread = tk_thread_create(input_run, info_dup(&info));
  if (thread != NULL) {
    tk_thread_start(thread);
  } else {
    close(info.fd);
    TKMEM_FREE(info.filename);
  }

  return thread;
}
