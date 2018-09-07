/**
 * File:   tslib_thread.c
 * Author: AWTK Develop Team
 * Brief:  thread to handle touch screen events
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
#include "tslib.h"
#include "base/mem.h"
#include "base/keys.h"
#include "base/thread.h"
#include "tslib_thread.h"

typedef struct _run_info_t {
  int32_t max_x;
  int32_t max_y;
  struct tsdev* ts;
  void* dispatch_ctx;
  input_dispatch_t dispatch;

  event_queue_req_t req;
} run_info_t;

static ret_t tslib_dispatch(run_info_t* info) {
  ret_t ret = info->dispatch(info->dispatch_ctx, &(info->req));
  info->req.event.type = EVT_NONE;

  return ret;
}

static ret_t tslib_dispatch_one_event(run_info_t* info) {
  struct ts_sample e = {0};
  event_queue_req_t* req = &(info->req);
  int ret = ret = ts_read(info->ts, &e, 1);

  if (ret <= 0) {
    return RET_OK;
  }

  return_value_if_fail(ret == sizeof(e), RET_FAIL);

  req->event.type = EVT_NONE;
  req->pointer_event.x = e.x;
  req->pointer_event.y = e.y;

  log_debug("%s: e.pressure=%d x=%d y=%d\n", __func__, e.pressure, e.x, e.y);

  if (e.pressure > 0) {
    if (req->pointer_event.pressed) {
      req->event.type = EVT_POINTER_MOVE;
    } else {
      req->event.type = EVT_POINTER_DOWN;
      req->pointer_event.pressed = 1;
    }
  } else {
    if (req->pointer_event.pressed) {
      req->event.type = EVT_POINTER_UP;
    }
    req->pointer_event.pressed = FALSE;
  }

  return tslib_dispatch(info);
}

void* tslib_run(void* ctx) {
  run_info_t* info = (run_info_t*)ctx;

  while (tslib_dispatch_one_event(info) == RET_OK)
    ;

  ts_close(info->ts);
  TKMEM_FREE(info);

  return NULL;
}

static run_info_t* info_dup(run_info_t* info) {
  run_info_t* new_info = TKMEM_ZALLOC(run_info_t);

  *new_info = *info;

  return new_info;
}

thread_t* tslib_thread_run(const char* filename, input_dispatch_t dispatch, void* ctx,
                           int32_t max_x, int32_t max_y) {
  run_info_t info;
  thread_t* thread = NULL;
  return_value_if_fail(filename != NULL && dispatch != NULL, NULL);

  memset(&info, 0x00, sizeof(info));

  info.max_x = max_x;
  info.max_y = max_y;
  info.dispatch_ctx = ctx;
  info.dispatch = dispatch;
  info.ts = ts_open(filename, 1);

  return_value_if_fail(info.ts != NULL, NULL);
  ts_config(info.ts);

  thread = thread_create(tslib_run, info_dup(&info));
  if (thread != NULL) {
    thread_start(thread);
  } else {
    ts_close(info.ts);
  }

  return thread;
}
