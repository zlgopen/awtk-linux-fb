/**
 * File:   input_dispatcher.h
 * Author: AWTK Develop Team
 * Brief:  interface to dispatch input events
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

#ifndef TK_INPUT_DISPATCHER_H
#define TK_INPUT_DISPATCHER_H

#include "base/event_queue.h"

BEGIN_C_DECLS

typedef ret_t (*input_dispatch_t)(void* ctx, const event_queue_req_t* e, const char* msg);

ret_t input_dispatch_print(void* ctx, const event_queue_req_t* e, const char* msg);

END_C_DECLS

#endif /*TK_INPUT_DISPATCHER_H*/
