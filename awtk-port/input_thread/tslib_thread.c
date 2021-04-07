/**
 * File:   tslib_thread.c
 * Author: AWTK Develop Team
 * Brief:  thread to handle touch screen events
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
#include "tslib.h"
#include "tkc/mem.h"
#include "base/keys.h"
#include "tkc/thread.h"
#include "tslib_thread.h"
#include "tkc/utils.h"

typedef struct _run_info_t {
  int32_t max_x;
  int32_t max_y;
  struct tsdev* ts;
  void* dispatch_ctx;
  char* filename;
  input_dispatch_t dispatch;

  event_queue_req_t req;
} run_info_t;

static ret_t tslib_dispatch(run_info_t* info) {
  ret_t ret = info->dispatch(info->dispatch_ctx, &(info->req), "tslib");
  info->req.event.type = EVT_NONE;

  return ret;
}

static ret_t tslib_dispatch_one_event(run_info_t* info) {
  struct ts_sample e = {0};
  int ret = -1;

  if (info->ts != NULL) {
    ret = ts_read(info->ts, &e, 1);
  }

  event_queue_req_t* req = &(info->req);

  if (ret == 0) {
    log_warn("%s:%d: get tslib data failed, filename=%s\n", __func__, __LINE__, info->filename);
    sleep(1);
    return RET_OK;
  } else if (ret < 0) {
    sleep(2);

    if (access(info->filename, R_OK) == 0) {
      if (info->ts != NULL) {
        ts_close(info->ts);
      }
      info->ts = ts_open(info->filename, 0);
      return_value_if_fail(info->ts != NULL, RET_OK);
      ts_config(info->ts);

      if (info->ts == NULL) {
        log_debug("%s:%d: open tslib failed, filename=%s\n", __func__, __LINE__, info->filename);
        perror("print tslib: ");
      } else {
        log_debug("%s:%d: open tslib successful, filename=%s\n", __func__, __LINE__,
                  info->filename);
      }
    }

    return RET_OK;
  }

  req->event.type = EVT_NONE;
  req->pointer_event.x = e.x;
  req->pointer_event.y = e.y;

  log_debug("%s%d: e.pressure=%d x=%d y=%d ret=%d\n", __func__, __LINE__, e.pressure, e.x, e.y,
            ret);

  if (e.pressure > 0) {
    if (req->pointer_event.pressed) {
      req->event.type = EVT_POINTER_MOVE;
    } else {
      req->event.type = EVT_POINTER_DOWN;
      req->pointer_event.pressed = TRUE;
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
  run_info_t info = *(run_info_t*)ctx;
  if (info.ts == NULL) {
    log_debug("%s:%d: open tslib failed, filename=%s\n", __func__, __LINE__, info.filename);
  } else {
    log_debug("%s:%d: open tslib successful, filename=%s\n", __func__, __LINE__, info.filename);
  }

  TKMEM_FREE(ctx);
  while (tslib_dispatch_one_event(&info) == RET_OK)
    ;
  ts_close(info.ts);
  TKMEM_FREE(info.filename);

  return NULL;
}

static run_info_t* info_dup(run_info_t* info) {
  run_info_t* new_info = TKMEM_ZALLOC(run_info_t);

  *new_info = *info;

  return new_info;
}

tk_thread_t* tslib_thread_run(const char* filename, input_dispatch_t dispatch, void* ctx,
                              int32_t max_x, int32_t max_y) {
  run_info_t info;
  tk_thread_t* thread = NULL;
  return_value_if_fail(filename != NULL && dispatch != NULL, NULL);

  memset(&info, 0x00, sizeof(info));

  info.max_x = max_x;
  info.max_y = max_y;
  info.dispatch_ctx = ctx;
  info.dispatch = dispatch;
  info.ts = ts_open(filename, 0);
  info.filename = tk_strdup(filename);

  if (info.ts != NULL) {
    ts_config(info.ts);
  }

  thread = tk_thread_create(tslib_run, info_dup(&info));
  if (thread != NULL) {
    tk_thread_start(thread);
  } else {
    ts_close(info.ts);
    TKMEM_FREE(info.filename);
  }

  return thread;
}
