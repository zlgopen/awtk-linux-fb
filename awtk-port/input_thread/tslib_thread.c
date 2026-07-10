/**
 * File:   tslib_thread.c
 * Author: AWTK Develop Team
 * Brief:  thread to handle touch screen events
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
#include <sys/ioctl.h>
#include <linux/input.h>
#include "tslib.h"
#include "tkc/mem.h"
#include "tkc/utils.h"
#include "tkc/thread.h"
#include "base/keys.h"

#include "tslib_thread.h"

#define TS_OPEN_NONBLOCK 1
#define TS_OPEN_BLOCK    0

typedef struct _run_info_t {
  int32_t max_x;
  int32_t max_y;
  struct tsdev* ts;
  void* dispatch_ctx;
  char* filename;
  input_dispatch_t dispatch;
  exit_notifier_t* exit_notifier;

  event_queue_req_t req;
} run_info_t;

static ret_t tslib_dispatch(run_info_t* info) {
  ret_t ret = RET_FAIL;
  char message[MAX_PATH + 1] = {0};
  tk_snprintf(message, sizeof(message) - 1, "ts[%s]", info->filename);

  ret = info->dispatch(info->dispatch_ctx, &(info->req), message);
  info->req.event.type = EVT_NONE;

  return ret;
}

/* If the touch device has been removed, then return 0 (errno==ENODEV) */
static int touch_device_alive(int fd) {
  int version;
  return ioctl(fd, EVIOCGVERSION, &version) == 0;
}

/* reopen the touch device, then init the info.ts */
static ret_t tslib_reopen(run_info_t* info) {
  assert(info);
  log_warn("reopen tslib, filename=%s\n", info->filename);

  if (info->ts != NULL) {
    ts_close(info->ts);
    info->ts = NULL;
  }

  info->ts = ts_open(info->filename, info->exit_notifier? TS_OPEN_NONBLOCK: TS_OPEN_BLOCK);
  if (info->ts != NULL) {
    ts_config(info->ts);
  }

  if (info->ts == NULL) {
    log_debug("%s:%d: open tslib failed, filename=%s\n", __func__, __LINE__, info->filename);
    perror("print tslib: ");
  } else {
    log_debug("%s:%d: open tslib successful, filename=%s\n", __func__, __LINE__, info->filename);
  }
  return RET_OK;
}

static ret_t tslib_dispatch_one_event(run_info_t* info) {
  struct ts_sample e = {0};
  int ret = -1;

  if (info->exit_notifier && exit_notifier_get_flag(info->exit_notifier)) {
    return RET_QUIT;
  }

  if (info->ts == NULL) {
    sleep(1);
    return tslib_reopen(info);
  }

  /* Use nonblock ts_read->wait->ts_read to fix issues/123 in tslib-1.1 variance module */
  ret = ts_read(info->ts, &e, 1);
  if (ret <= 0) {
    if (info->exit_notifier && exit_notifier_wait(info->exit_notifier, ts_fd(info->ts)) != ts_fd(info->ts)) {
      /*
      If the fd device is unplugged, wait function will still return the fd and read return -1
      */
      if (exit_notifier_get_flag(info->exit_notifier))
        return RET_QUIT;
    }
    ret = ts_read(info->ts, &e, 1);
  }

  if (ret <= 0) {
    log_warn("%s:%d: get tslib data failed, filename=%s\n", __func__, __LINE__, info->filename);

    if (!touch_device_alive(ts_fd(info->ts))) {
      return tslib_reopen(info);
    }

    /* Touch data incomplete, continue to next ts_read */
    return RET_OK;
  }

  event_queue_req_t* req = &(info->req);
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
  while (tslib_dispatch_one_event(&info) == RET_OK) {
  };
  if (info.ts != NULL) {
    ts_close(info.ts);
  }
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
  return tslib_thread_run_ex(filename, dispatch, ctx, max_x, max_y, NULL);
}

tk_thread_t* tslib_thread_run_ex(const char* filename, input_dispatch_t dispatch, void* ctx,
                              int32_t max_x, int32_t max_y, exit_notifier_t* exit_notifier) {
  run_info_t info;
  tk_thread_t* thread = NULL;
  return_value_if_fail(filename != NULL && dispatch != NULL, NULL);

  memset(&info, 0x00, sizeof(info));

  info.max_x = max_x;
  info.max_y = max_y;
  info.dispatch_ctx = ctx;
  info.dispatch = dispatch;
  info.exit_notifier = exit_notifier;
  info.ts = ts_open(filename, exit_notifier? TS_OPEN_NONBLOCK: TS_OPEN_BLOCK);
  info.filename = tk_strdup(filename);

  if (info.ts != NULL) {
    ts_config(info.ts);
  }

  thread = tk_thread_create(tslib_run, info_dup(&info));
  if (thread != NULL) {
    tk_thread_start(thread);
  } else {
    if (info.ts != NULL) {
      ts_close(info.ts);
    }
    TKMEM_FREE(info.filename);
  }

  return thread;
}
