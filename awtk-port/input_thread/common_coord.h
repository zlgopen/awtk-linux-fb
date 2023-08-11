/**
 * File:   common_coord.h
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

#ifndef TK_COMMON_COORD_H
#define TK_COMMON_COORD_H

#include "tkc/types_def.h"
#include "tkc/rect.h"

BEGIN_C_DECLS

ret_t common_coord_init(void);

ret_t common_coord_get(point_t* p_coord);

ret_t common_coord_set(point_t coord);

ret_t common_coord_deinit(void);

END_C_DECLS

#endif /*TK_COMMON_COORD_H*/
