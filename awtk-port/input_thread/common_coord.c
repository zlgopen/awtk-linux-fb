/**
 * File:   common_coord.c
 * Author: AWTK Develop Team
 * Brief:  公共坐标
 *
 * Copyright (c) 2018 - 2025 Guangzhou ZHIYUAN Electronics Co.,Ltd.
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
#include "tkc/atomic.h"

static tk_atomic_t s_coord;

#define COMMON_COORD_POINT_TO_U64(p) (((uint64_t)p.x << 32) | p.y)
#define COMMON_COORD_POINT_FROM_U64(p) \
  point_init((xy_t)((num >> 32) & 0xFFFFFFFF), (xy_t)(num & 0xFFFFFFFF))

ret_t common_coord_init(void) {
  value_t v;
  ret_t ret = tk_atomic_init(&s_coord, value_set_uint64(&v, 0));
  return_value_if_fail(RET_OK == ret, ret);
  return ret;
}

ret_t common_coord_get(point_t* p_coord) {
  value_t v;
  ret_t ret = RET_FAIL;
  return_value_if_fail(p_coord != NULL, RET_BAD_PARAMS);

  ret = tk_atomic_load(&s_coord, &v);
  if (RET_OK == ret) {
    uint64_t num = value_uint64(&v);
    *p_coord = COMMON_COORD_POINT_FROM_U64(num);
  }

  return ret;
}

ret_t common_coord_set(point_t coord) {
  value_t v;
  return tk_atomic_store(&s_coord, value_set_uint64(&v, COMMON_COORD_POINT_TO_U64(coord)));
}

ret_t common_coord_deinit(void) {
  ret_t ret = tk_atomic_deinit(&s_coord);
  return_value_if_fail(RET_OK == ret, ret);
  return ret;
}
