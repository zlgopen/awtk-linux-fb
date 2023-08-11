/**
 * File:   common_coord.c
 * Author: AWTK Develop Team
 * Brief:  公共坐标
 *
 * Copyright (c) 2018 - 2023  Guangzhou ZHIYUAN Electronics Co.,Ltd.
 *
 * this program is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose.  see the
 * license file for more details.
 *
 */

/**
 * history:
 * ================================================================
 * 2023-08-11 Shen ZhaoKun <shenzhaokun@zlg.cn> created
 *
 */

#include "common_coord.h"
#include "tkc/mutex.h"

static point_t s_coord = {0, 0};
static tk_mutex_t* s_mutex = NULL;

ret_t common_coord_init(void) {
  s_mutex = tk_mutex_create();
  goto_error_if_fail(s_mutex != NULL);
  return RET_OK;
error:
  return RET_FAIL;
}

ret_t common_coord_get(point_t* p_coord) {
  ret_t ret = RET_FAIL;
  return_value_if_fail(p_coord != NULL, RET_BAD_PARAMS);
  return_value_if_fail(s_mutex != NULL, ret);

  ret = tk_mutex_try_lock(s_mutex);
  if (RET_OK == ret) {
    *p_coord = s_coord;
    tk_mutex_unlock(s_mutex);
  }

  return ret;
}

ret_t common_coord_set(point_t coord) {
  ret_t ret = RET_FAIL;
  return_value_if_fail(s_mutex != NULL, ret);

  ret = tk_mutex_try_lock(s_mutex);
  if (RET_OK == ret) {
    s_coord = coord;
    tk_mutex_unlock(s_mutex);
  }

  return ret;
}

ret_t common_coord_deinit(void) {
  if (s_mutex != NULL) {
    tk_mutex_destroy(s_mutex);
    s_mutex = NULL;
  }
  return RET_OK;
}
