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


static const int32_t s_key_map[0x100] = {[KEY_1] = TK_KEY_1,
                                         [KEY_2] = TK_KEY_2,
                                         [KEY_3] = TK_KEY_3,
                                         [KEY_4] = TK_KEY_4,
                                         [KEY_5] = TK_KEY_5,
                                         [KEY_6] = TK_KEY_6,
                                         [KEY_7] = TK_KEY_7,
                                         [KEY_8] = TK_KEY_8,
                                         [KEY_9] = TK_KEY_9,
                                         [KEY_0] = TK_KEY_0,
                                         [KEY_A] = TK_KEY_a,
                                         [KEY_B] = TK_KEY_b,
                                         [KEY_C] = TK_KEY_c,
                                         [KEY_D] = TK_KEY_d,
                                         [KEY_E] = TK_KEY_e,
                                         [KEY_F] = TK_KEY_f,
                                         [KEY_G] = TK_KEY_g,
                                         [KEY_H] = TK_KEY_h,
                                         [KEY_I] = TK_KEY_i,
                                         [KEY_J] = TK_KEY_j,
                                         [KEY_K] = TK_KEY_k,
                                         [KEY_L] = TK_KEY_l,
                                         [KEY_M] = TK_KEY_m,
                                         [KEY_N] = TK_KEY_n,
                                         [KEY_O] = TK_KEY_o,
                                         [KEY_P] = TK_KEY_p,
                                         [KEY_Q] = TK_KEY_q,
                                         [KEY_R] = TK_KEY_r,
                                         [KEY_S] = TK_KEY_s,
                                         [KEY_T] = TK_KEY_t,
                                         [KEY_U] = TK_KEY_u,
                                         [KEY_V] = TK_KEY_v,
                                         [KEY_W] = TK_KEY_w,
                                         [KEY_X] = TK_KEY_x,
                                         [KEY_Y] = TK_KEY_y,
                                         [KEY_Z] = TK_KEY_z,
                                         [KEY_RIGHTCTRL] = TK_KEY_RCTRL,
                                         [KEY_RIGHTALT] = TK_KEY_RALT,
                                         [KEY_HOME] = TK_KEY_HOME,
                                         [KEY_UP] = TK_KEY_UP,
                                         [KEY_PAGEUP] = TK_KEY_PAGEUP,
                                         [KEY_LEFT] = TK_KEY_LEFT,
                                         [KEY_RIGHT] = TK_KEY_RIGHT,
                                         [KEY_END] = TK_KEY_END,
                                         [KEY_DOWN] = TK_KEY_DOWN,
                                         [KEY_PAGEDOWN] = TK_KEY_PAGEDOWN,
                                         [KEY_INSERT] = TK_KEY_INSERT,
                                         [KEY_DELETE] = TK_KEY_DELETE,
                                         [KEY_F1] = TK_KEY_F1,
                                         [KEY_F2] = TK_KEY_F2,
                                         [KEY_F3] = TK_KEY_F3,
                                         [KEY_F4] = TK_KEY_F4,
                                         [KEY_F5] = TK_KEY_F5,
                                         [KEY_F6] = TK_KEY_F6,
                                         [KEY_F7] = TK_KEY_F7,
                                         [KEY_F8] = TK_KEY_F8,
                                         [KEY_F9] = TK_KEY_F9,
                                         [KEY_F10] = TK_KEY_F10,
                                         [KEY_F11] = TK_KEY_F11,
                                         [KEY_F12] = TK_KEY_F12,
                                         [KEY_COMMA] = TK_KEY_COMMA,
                                         [KEY_DOT] = TK_KEY_DOT,
                                         [KEY_SLASH] = TK_KEY_SLASH,
                                         [KEY_RIGHTSHIFT] = TK_KEY_RSHIFT,
                                         [KEY_LEFTALT] = TK_KEY_LALT,
                                         [KEY_SPACE] = TK_KEY_SPACE,
                                         [KEY_CAPSLOCK] = TK_KEY_CAPSLOCK,
                                         [KEY_SEMICOLON] = TK_KEY_SEMICOLON,
                                         [KEY_LEFTSHIFT] = TK_KEY_LSHIFT,
                                         [KEY_BACKSLASH] = TK_KEY_BACKSLASH,
                                         [KEY_LEFTBRACE] = TK_KEY_LEFTBRACE,
                                         [KEY_RIGHTBRACE] = TK_KEY_RIGHTBRACE,
                                         [KEY_ENTER] = TK_KEY_SPACE,
                                         [KEY_LEFTCTRL] = TK_KEY_LCTRL,
                                         [KEY_MINUS] = TK_KEY_MINUS,
                                         [KEY_EQUAL] = TK_KEY_EQUAL,
                                         [KEY_BACKSPACE] = TK_KEY_BACKSPACE,
                                         [KEY_TAB] = TK_KEY_TAB,
                                         [KEY_ESC] = TK_KEY_ESCAPE};

int32_t map_key(uint8_t code) {
  return s_key_map[code];
}
