/**
 * File:   mouse_thread.c
 * Author: AWTK Develop Team
 * Brief:  thread to read /dev/input/
 *
 * Copyright (c) 2018 - 2018  Guangzhou ZHIYUAN Electronics Co.,Ltd.
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
#include "base/mem.h"
#include "base/keys.h"
#include "base/thread.h"
#include "mouse_thread.h"

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
  input_dispatch_t dispatch;

  event_queue_req_t req;
} run_info_t;

static ret_t input_dispatch(run_info_t* info) {
  ret_t ret = info->dispatch(info->dispatch_ctx, &(info->req));
  info->req.event.type = EVT_NONE;

  return ret;
}

static ret_t input_dispatch_one_event(run_info_t* info) {
  int8_t data[3];
  event_queue_req_t* req = &(info->req);
  int ret = read(info->fd, data, sizeof(data));

  if(ret == 3) {
    int left = data[0] & 0x1;
    //int right = data[0] & 0x2;
    //int middle = data[0] & 0x4;
    int x = data[1];
    int y = data[2];

    info->x += x;
    info->y -= y;

    if(info->x < 0) {
      info->x = 0;
    }
    if(info->x > info->max_x) {
      info->x = info->max_x;
    }
    if(info->y < 0) {
      info->y = 0;
    }
    if(info->y > info->max_y) {
      info->y = info->max_y;
    }

    if(left) {
      if(!req->pointer_event.pressed) {
        req->pointer_event.pressed = TRUE;
        req->event.type = EVT_POINTER_DOWN;
      } else {
        req->event.type = EVT_POINTER_MOVE;
      }
    } else {
      if(req->pointer_event.pressed) {
        req->pointer_event.pressed = FALSE;
        req->event.type = EVT_POINTER_UP;
      } else {
        req->event.type = EVT_POINTER_MOVE;
      }
    }
    req->pointer_event.x = info->x;
    req->pointer_event.y = info->y;

    input_dispatch(info);
  }

  return RET_OK;
}

void* input_run(void* ctx) {
  run_info_t* info = (run_info_t*)ctx;

  while (input_dispatch_one_event(info) == RET_OK)
    ;

  close(info->fd);
  TKMEM_FREE(info);

  return NULL;
}

static run_info_t* info_dup(run_info_t* info) {
  run_info_t* new_info = TKMEM_ZALLOC(run_info_t);

  *new_info = *info;

  return new_info;
}

thread_t* mouse_thread_run(const char* filename, input_dispatch_t dispatch, void* ctx,
                           int32_t max_x, int32_t max_y) {
  run_info_t info;
  thread_t* thread = NULL;
  return_value_if_fail(filename != NULL && dispatch != NULL, NULL);

  memset(&info, 0x00, sizeof(info));

  info.max_x = max_x;
  info.max_y = max_y;
  info.dispatch_ctx = ctx;
  info.dispatch = dispatch;
  info.fd = open(filename, O_RDONLY);

  return_value_if_fail(info.fd >= 0, NULL);

  thread = thread_create(input_run, info_dup(&info));
  if (thread != NULL) {
    thread_start(thread);
  } else {
    close(info.fd);
  }

  return thread;
}
