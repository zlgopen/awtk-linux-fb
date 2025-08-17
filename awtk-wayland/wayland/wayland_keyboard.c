/**
 * File:   wayland_keyboard.c
 * Author: AWTK Develop Team
 * Brief:  wayland map key
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
#include "tkc/mem.h"
#include "base/keys.h"
#include "tkc/thread.h"

#ifndef EV_SYN
#define EV_SYN 0x00
#endif

#include "key_map.inc"

