/**
 * File:   input_thread.c
 * Author: AWTK Develop Team
 * Brief:  thread to read /dev/input/
 *
 * Copyright (c) 2018 - 2025 Guangzhou ZHIYUAN Electronics Co.,Ltd.
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
#include "tkc/utils.h"
#include "tkc/thread.h"
#include "base/keys.h"

#include "input_thread.h"
#include "common_coord.h"

#ifndef EV_SYN
#define EV_SYN 0x00
#endif /*EV_SYN*/

#ifndef ABS_MT_SLOT
#define ABS_MT_SLOT 0x2f
#endif /*ABS_MT_SLOT*/

#ifndef ABS_MT_PRESSURE
#define ABS_MT_PRESSURE 0x3a
#endif /*ABS_MT_PRESSURE*/

#define AWTK_FINGER_ID_START 1000
#define AWTK_MAX_FINGERS 10

#include "key_map.inc"

typedef struct _slot_finger_info_t {
  int32_t id;
  int32_t x;
  int32_t y;
} slot_finger_info_t;

typedef struct _run_info_t {
  int fd;
  int32_t max_x;
  int32_t max_y;
  int32_t last_x;
  int32_t last_y;
  int32_t last_event_type;
  void* dispatch_ctx;
  char* filename;
  input_dispatch_t dispatch;

  bool_t pressed;
  bool_t is_mod_inited;
  bool_t capslock;
  bool_t numlock;
  event_queue_req_t req;
  bool_t is_single_touch;

  uint32_t active_finger;
  slot_finger_info_t fingers[AWTK_MAX_FINGERS];
} run_info_t;

static bool_t run_info_is_no_finger_down(run_info_t* info) {
  uint32_t i = 0;
  for (i = 0; i < AWTK_MAX_FINGERS; i++) {
    if (info->fingers[i].id > 0) {
      return FALSE;
    }
  }

  return TRUE;
}

static run_info_t* run_info_create(const char* filename, input_dispatch_t dispatch, void* dispatch_ctx, int32_t max_x, int32_t max_y) {
  run_info_t* info = NULL;
  return_value_if_fail(filename != NULL && dispatch != NULL, NULL);
  info = TKMEM_ZALLOC(run_info_t);
  return_value_if_fail(info != NULL, NULL);
  info->fd = open(filename, O_RDONLY);
  info->max_x = max_x;
  info->max_y = max_y;
  info->dispatch = dispatch;
  info->dispatch_ctx = dispatch_ctx;
  info->filename = tk_strdup(filename);

  return info;
}

static ret_t run_info_destroy(run_info_t* info) {
  return_value_if_fail(info != NULL, RET_BAD_PARAMS);
  if (info->fd > 0) {
    close(info->fd);
  }
  TKMEM_FREE(info->filename);
  TKMEM_FREE(info);

  return RET_OK;
}


#define test_bit(bmap, idx) (((bmap)[(idx) / 8] & (1 << ((idx) % 8))) != 0)

/*
 * 因为有些Linux内核中KEY_CAPSLOCK(58)映射的是CtrlL_Lock而不是Caps_Lock，导致LED_CAPSL不生效，
 * 所以capslock在KEY_CAPSLOCK松开时修改，而不直接使用LED_CAPSL状态。
 */
static ret_t input_init_mod(run_info_t* info) {
  if (!info->is_mod_inited) {
    uint8_t led_mask[LED_MAX / 8 + 1];
    memset(&led_mask, 0, sizeof(led_mask));
    ioctl(info->fd, EVIOCGLED(sizeof(led_mask)), led_mask);

    info->capslock = FALSE;
    info->numlock = test_bit(led_mask, LED_NUML);
    info->is_mod_inited = TRUE;
  }

  return RET_OK;
}

static ret_t input_dispatch(run_info_t* info) {
  ret_t ret = RET_FAIL;
  char message[MAX_PATH + 1] = {0};
  
  if (!info->req.event.type) {
    return RET_OK;
  }

  tk_snprintf(message, sizeof(message) - 1, "input[%s]", info->filename);
  if (info->req.event.type == EVT_POINTER_MOVE) {
    if (info->last_event_type == EVT_POINTER_MOVE) {
      if (info->req.pointer_event.x == info->last_x && info->req.pointer_event.y == info->last_y) {
        log_debug("skip repeat event\n");
        return RET_OK;
      }
    }
    
    info->last_x = info->req.pointer_event.x;
    info->last_y = info->req.pointer_event.y;
  }

  ret = info->dispatch(info->dispatch_ctx, &(info->req), message);
 
  if (!info->is_single_touch) {
    int event_type = info->req.event.type;
    if (event_type == EVT_POINTER_DOWN || event_type == EVT_POINTER_MOVE || event_type == EVT_POINTER_UP) {
      event_queue_req_t req;
      slot_finger_info_t* finger = info->fingers + info->active_finger;
      float x = (float)(finger->x) / (float)(info->max_x);
      float y = (float)(finger->y) / (float)(info->max_y);
      int32_t finger_id = finger->id;

      memset(&req, 0x00, sizeof(req));
      if (event_type == EVT_POINTER_DOWN) {
        event_type = EVT_TOUCH_DOWN; 
      } else if (event_type == EVT_POINTER_MOVE) {
        event_type = EVT_TOUCH_MOVE; 
      } else if (event_type == EVT_POINTER_UP) {
        event_type = EVT_TOUCH_UP; 
        finger->id = -1;
      }

      req.event.type = event_type;
      touch_event_init(&req.touch_event, event_type, NULL, 0, finger_id, x, y, 0);
      info->dispatch(info->dispatch_ctx, &req, message);

      if (event_type == EVT_TOUCH_UP && run_info_is_no_finger_down(info)) {
        touch_event_init(&req.touch_event, event_type, NULL, 0, -1, x, y, 0);
        info->dispatch(info->dispatch_ctx, &req, message);
      }
    }
  }
  info->last_event_type = info->req.event.type;
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

  input_init_mod(info);

  //log_debug("input_event e.type=0x%x code=0x%x value=0x%x\n", e.type, e.code, e.value);

  switch (e.type) {
    case EV_LED: {
      switch (e.code) {
        case LED_CAPSL: {
          // capslock不直接使用LED_CAPSL状态
          break;
        }
        case LED_NUML: {
          info->numlock = e.value != 0;
          break;
        }
        default: {
          log_info("unkown code: e.type=%d code=%d value=%d\n", e.type, e.code, e.value);
          break;
        }
      }

      break;
    }
    case EV_KEY: {
      if (e.code == BTN_LEFT || e.code == BTN_RIGHT || e.code == BTN_MIDDLE ||
          e.code == BTN_TOUCH) {
        req->event.type = e.value ? EVT_POINTER_DOWN : EVT_POINTER_UP;
        info->is_single_touch = TRUE;
      } else {
        // capslock在KEY_CAPSLOCK松开时修改
        if (e.code == KEY_CAPSLOCK && !e.value) {
          info->capslock = !info->capslock;
          log_warn("capslock is changed by key event and now is %d\n", info->capslock);
        }

        req->event.type = e.value ? EVT_KEY_DOWN : EVT_KEY_UP;
        req->key_event.key = map_key(e.code);
        req->key_event.capslock = info->capslock;
        req->key_event.numlock = info->numlock;

        return input_dispatch(info);
      }

      break;
    }
    case EV_ABS: {
      switch (e.code) {
        case ABS_MT_POSITION_X:
        case ABS_X: {
          req->pointer_event.x = e.value;
          info->fingers[info->active_finger].x = e.value;
          break;
        }
        case ABS_MT_POSITION_Y:
        case ABS_Y: {
          req->pointer_event.y = e.value;
          info->fingers[info->active_finger].y = e.value;
          break;
        }
        case ABS_MT_TRACKING_ID: {
          log_debug("ABS_MT_TRACKING_ID:%d\n", e.value);
          if (e.value < 0) {
            req->pointer_event.finger_id = e.value;
          } else {
            req->pointer_event.finger_id = e.value + AWTK_FINGER_ID_START;
            info->fingers[info->active_finger].id = e.value + AWTK_FINGER_ID_START;
          }

          if (!info->is_single_touch) {
            if (e.value == -1) {
              req->event.type = EVT_POINTER_UP;
            } else {
              req->event.type = EVT_POINTER_DOWN;
            }        
          }
          break;
        }
        case ABS_MT_SLOT: {
          log_debug("ABS_MT_SLOT:%d\n", e.value);
          info->active_finger = e.value;
          assert(info->active_finger < AWTK_MAX_FINGERS);
          break;
        }
        case ABS_MT_TOUCH_MAJOR:
        case ABS_MT_TOUCH_MINOR:
        case ABS_MT_WIDTH_MAJOR:
        case ABS_MT_WIDTH_MINOR:
        case ABS_MT_PRESSURE:
        case ABS_MT_BLOB_ID: {
          log_info("ignore: e.type=%d code=%d value=%d\n", e.type, e.code, e.value);
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
      point_t common_coord = {req->pointer_event.x, req->pointer_event.y};
      if (RET_OK == common_coord_get(&common_coord)) {
        req->pointer_event.x = common_coord.x;
        req->pointer_event.y = common_coord.y;
      }
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
  run_info_t* info = (run_info_t*)ctx;

  while (input_dispatch_one_event(info) == RET_OK) {
  };
  
  run_info_destroy(info);

  return NULL;
}

tk_thread_t* input_thread_run(const char* filename, input_dispatch_t dispatch, void* ctx,
                              int32_t max_x, int32_t max_y) {
  tk_thread_t* thread = NULL;
  run_info_t* info = run_info_create(filename, dispatch, ctx, max_x, max_y);
  return_value_if_fail(info != NULL, NULL);

  thread = tk_thread_create(input_run, info);
  if (thread != NULL) {
    tk_thread_start(thread);
  } else {
    run_info_destroy(info);
  }

  return thread;
}

ret_t input_thread_global_init(void) {
  custom_keys_init(FALSE);
  return RET_OK;
}

ret_t input_thread_global_deinit(void) {
  custom_keys_deinit(FALSE);
  return RET_OK;
}
