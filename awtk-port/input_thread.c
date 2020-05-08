/**
 * File:   input_thread.c
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
#include "input_thread.h"
#include "tkc/utils.h"

#ifndef EV_SYN
#define EV_SYN 0x00
#endif /*EV_SYN*/

#ifndef ABS_MT_SLOT
#define ABS_MT_SLOT 0x2f
#endif /*ABS_MT_SLOT*/

#ifndef ABS_MT_PRESSURE
#define ABS_MT_PRESSURE 0x3a
#endif /*ABS_MT_PRESSURE*/

typedef struct _run_info_t {
  int fd;
  int32_t max_x;
  int32_t max_y;
  void* dispatch_ctx;
  char* filename;
  input_dispatch_t dispatch;

  bool_t pressed;
  event_queue_req_t req;
} run_info_t;

static const int32_t s_key_map[0x100] = {[KEY_1] = TK_KEY_1,
                                         [KEY_2] = TK_KEY_2,
                                         [KEY_3] = TK_KEY_3,
                                         [KEY_4] = TK_KEY_4,
                                         [KEY_5] = TK_KEY_5,
                                         [KEY_6] = TK_KEY_6,
                                         [KEY_7] = TK_KEY_7,
                                         [KEY_8] = TK_KEY_8,
                                         [KEY_9] = TK_KEY_9,
                                         [KEY_0] = TK_KEY_0,
                                         [KEY_A] = TK_KEY_a,
                                         [KEY_B] = TK_KEY_b,
                                         [KEY_C] = TK_KEY_c,
                                         [KEY_D] = TK_KEY_d,
                                         [KEY_E] = TK_KEY_e,
                                         [KEY_F] = TK_KEY_f,
                                         [KEY_G] = TK_KEY_g,
                                         [KEY_H] = TK_KEY_h,
                                         [KEY_I] = TK_KEY_i,
                                         [KEY_J] = TK_KEY_j,
                                         [KEY_K] = TK_KEY_k,
                                         [KEY_L] = TK_KEY_l,
                                         [KEY_M] = TK_KEY_m,
                                         [KEY_N] = TK_KEY_n,
                                         [KEY_O] = TK_KEY_o,
                                         [KEY_P] = TK_KEY_p,
                                         [KEY_Q] = TK_KEY_q,
                                         [KEY_R] = TK_KEY_r,
                                         [KEY_S] = TK_KEY_s,
                                         [KEY_T] = TK_KEY_t,
                                         [KEY_U] = TK_KEY_u,
                                         [KEY_V] = TK_KEY_v,
                                         [KEY_W] = TK_KEY_w,
                                         [KEY_X] = TK_KEY_x,
                                         [KEY_Y] = TK_KEY_y,
                                         [KEY_Z] = TK_KEY_z,
                                         [KEY_RIGHTCTRL] = TK_KEY_RCTRL,
                                         [KEY_RIGHTALT] = TK_KEY_RALT,
                                         [KEY_HOME] = TK_KEY_HOME,
                                         [KEY_UP] = TK_KEY_UP,
                                         [KEY_PAGEUP] = TK_KEY_PAGEUP,
                                         [KEY_LEFT] = TK_KEY_LEFT,
                                         [KEY_RIGHT] = TK_KEY_RIGHT,
                                         [KEY_END] = TK_KEY_END,
                                         [KEY_DOWN] = TK_KEY_DOWN,
                                         [KEY_PAGEDOWN] = TK_KEY_PAGEDOWN,
                                         [KEY_INSERT] = TK_KEY_INSERT,
                                         [KEY_DELETE] = TK_KEY_DELETE,
                                         [KEY_F1] = TK_KEY_F1,
                                         [KEY_F2] = TK_KEY_F2,
                                         [KEY_F3] = TK_KEY_F3,
                                         [KEY_F4] = TK_KEY_F4,
                                         [KEY_F5] = TK_KEY_F5,
                                         [KEY_F6] = TK_KEY_F6,
                                         [KEY_F7] = TK_KEY_F7,
                                         [KEY_F8] = TK_KEY_F8,
                                         [KEY_F9] = TK_KEY_F9,
                                         [KEY_F10] = TK_KEY_F10,
                                         [KEY_F11] = TK_KEY_F11,
                                         [KEY_F12] = TK_KEY_F12,
                                         [KEY_COMMA] = TK_KEY_COMMA,
                                         [KEY_DOT] = TK_KEY_DOT,
                                         [KEY_SLASH] = TK_KEY_SLASH,
                                         [KEY_RIGHTSHIFT] = TK_KEY_RSHIFT,
                                         [KEY_LEFTALT] = TK_KEY_LALT,
                                         [KEY_SPACE] = TK_KEY_SPACE,
                                         [KEY_CAPSLOCK] = TK_KEY_CAPSLOCK,
                                         [KEY_SEMICOLON] = TK_KEY_SEMICOLON,
                                         [KEY_LEFTSHIFT] = TK_KEY_LSHIFT,
                                         [KEY_BACKSLASH] = TK_KEY_BACKSLASH,
                                         [KEY_LEFTBRACE] = TK_KEY_LEFTBRACE,
                                         [KEY_RIGHTBRACE] = TK_KEY_RIGHTBRACE,
                                         //[KEY_ENTER] = TK_KEY_ENTER,
                                         [KEY_LEFTCTRL] = TK_KEY_LCTRL,
                                         [KEY_MINUS] = TK_KEY_MINUS,
                                         [KEY_EQUAL] = TK_KEY_EQUAL,
                                         [KEY_BACKSPACE] = TK_KEY_BACKSPACE,
                                         [KEY_TAB] = TK_KEY_TAB,
                                         [KEY_ESC] = TK_KEY_ESCAPE};

static int32_t map_key(uint8_t code) {
  return s_key_map[code];
}

static ret_t input_dispatch(run_info_t* info) {
  ret_t ret = info->dispatch(info->dispatch_ctx, &(info->req), "input");
  info->req.event.type = EVT_NONE;

  return ret;
}

static ret_t input_dispatch_one_event(run_info_t* info) {
  int ret = 0;
  struct input_event e;
  event_queue_req_t* req = &(info->req);

  if (info->fd < 0) {
    ret = -1;
  } else {
    ret = read(info->fd, &e, sizeof(e));
  }

  if (ret < 0) {
    sleep(2);

    if (access(info->filename, R_OK) == 0) {
      if (info->fd >= 0) {
        close(info->fd);
      }

      info->fd = open(info->filename, O_RDONLY);
      if (info->fd < 0) {
        log_debug("%s:%d: open keyboard failed, fd=%d, filename=%s\n", __func__, __LINE__, info->fd,
                  info->filename);
        perror("print keyboard: ");
      } else {
        log_debug("%s:%d: open keyboard successful, fd=%d, filename=%s\n", __func__, __LINE__,
                  info->fd, info->filename);
      }
    } else {
      return RET_OK;
    }
  }

  return_value_if_fail(ret == sizeof(e), RET_OK);

  switch (e.type) {
    case EV_KEY: {
      if (e.code == BTN_LEFT || e.code == BTN_RIGHT || e.code == BTN_MIDDLE ||
          e.code == BTN_TOUCH) {
        req->event.type = e.value ? EVT_POINTER_DOWN : EVT_POINTER_UP;
      } else {
        req->event.type = e.value ? EVT_KEY_DOWN : EVT_KEY_UP;
        req->key_event.key = map_key(e.code);

        return input_dispatch(info);
      }

      break;
    }
    case EV_ABS: {
      switch (e.code) {
        case ABS_MT_POSITION_X:
        case ABS_X: {
          req->pointer_event.x = e.value;
          break;
        }
        case ABS_MT_POSITION_Y:
        case ABS_Y: {
          req->pointer_event.y = e.value;
          break;
        }
        case ABS_MT_TRACKING_ID: {
          if (e.value > 0) {
            req->event.type = EVT_POINTER_DOWN;
          } else {
            req->event.type = EVT_POINTER_UP;
          }
          break;
        }
        case ABS_MT_SLOT:
        case ABS_MT_TOUCH_MAJOR:
        case ABS_MT_TOUCH_MINOR:
        case ABS_MT_WIDTH_MAJOR:
        case ABS_MT_WIDTH_MINOR:
        case ABS_MT_PRESSURE:
        case ABS_MT_BLOB_ID: {
          break;
        }
        default: {
          log_info("unkown code: e.type=%d code=%d value=%d\n", e.type, e.code, e.value);
          break;
        }
      }

      if (req->event.type == EVT_NONE) {
        req->event.type = EVT_POINTER_MOVE;
      }

      break;
    }
    case EV_REL: {
      switch (e.code) {
        case REL_X: {
          req->pointer_event.x += e.value;

          if (req->pointer_event.x < 0) {
            req->pointer_event.x = 0;
          }
          if (req->pointer_event.x > info->max_x) {
            req->pointer_event.x = info->max_x;
          }
          break;
        }
        case REL_Y: {
          req->pointer_event.y += e.value;
          if (req->pointer_event.y < 0) {
            req->pointer_event.y = 0;
          }
          if (req->pointer_event.y > info->max_y) {
            req->pointer_event.y = info->max_y;
          }
          break;
        }
        default: {
          log_info("unkown code: e.type=%d code=%d value=%d\n", e.type, e.code, e.value);
          break;
        }
      }

      if (req->event.type == EVT_NONE) {
        req->event.type = EVT_POINTER_MOVE;
      }

      break;
    }
    case EV_SYN: {
      switch (req->event.type) {
        case EVT_POINTER_DOWN: {
          info->pressed = TRUE;
          req->pointer_event.pressed = TRUE;
          break;
        }
        case EVT_POINTER_MOVE: {
          req->pointer_event.pressed = info->pressed;
          break;
        }
        case EVT_POINTER_UP: {
          info->pressed = FALSE;
          req->pointer_event.pressed = TRUE;
          break;
        }
        default: {
          log_info("unkown code: e.type=%d code=%d value=%d\n", e.type, e.code, e.value);
          break;
        }
      }

      return input_dispatch(info);
    }
    default: {
      log_info("unkown type: e.type=%d code=%d value=%d\n", e.type, e.code, e.value);
      break;
    }
  }

  return RET_OK;
}

static void* input_run(void* ctx) {
  run_info_t info = *(run_info_t*)ctx;

  if (info.fd < 0) {
    log_debug("%s:%d: open keyboard failed, fd=%d, filename=%s\n", __func__, __LINE__, info.fd,
              info.filename);
  } else {
    log_debug("%s:%d: open keyboard successful, fd=%d, filename=%s\n", __func__, __LINE__, info.fd,
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

tk_thread_t* input_thread_run(const char* filename, input_dispatch_t dispatch, void* ctx,
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
