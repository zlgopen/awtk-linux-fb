/**
 * File:   wayland_keyboard.c
 * Author: AWTK Develop Team
 * Brief:  thread to read /dev/input/
 *
 * Copyright (c) 2018 - 2024 Guangzhou ZHIYUAN Electronics Co.,Ltd.
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
 * 2024-07-17 Yang Zewu <yangzewu@zlg.cn> created
 *
 */
#include "wayland_tools.h"

#define POINTER_BUTTON_STATE_MOTION -1

void keymap_format_cb(void *data, struct wl_keyboard *keyboard, uint32_t format,
                       int32_t fd, uint32_t keymap_size ) {
  struct keyboard *kbd = data;
  char *str = mmap (NULL, keymap_size, PROT_READ, MAP_SHARED, fd, 0);
  xkb_keymap_unref (kbd->map);
  kbd->map = xkb_keymap_new_from_string (kbd->ctx, str, XKB_KEYMAP_FORMAT_TEXT_V1,
                                         XKB_KEYMAP_COMPILE_NO_FLAGS);
  munmap (str, keymap_size);
  close (fd);
  xkb_state_unref (kbd->kb_state);
  //查看是否有释放new的内存
  kbd->kb_state = xkb_state_new (kbd->map);
}

extern void key_input(int,int);
void key_cb(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time,
             uint32_t key, uint32_t state) {
  (void) keyboard;
  (void) serial;
  (void) time;
  struct keyboard *keydata = data;
  if(keydata->kb_xcb){
    keydata->kb_xcb(state, key);
  }
}

static void motion_pointer_cb(void *data, struct wl_pointer *pointer, uint32_t time,
                               wl_fixed_t surface_x, wl_fixed_t surface_y) {
  struct point *point = data;
  if(point->point_xcb){
    point->point_xcb(POINTER_BUTTON_STATE_MOTION, 0,
                     wl_fixed_to_int(surface_x), wl_fixed_to_int(surface_y));
  }
}

static void button_pointer_cb(void *data, struct wl_pointer *pointer, uint32_t serial,
                               uint32_t time, uint32_t button, uint32_t state) {
  struct point *point = data;

  if (point->point_xcb) {
    point->point_xcb(state, button, -1, -1);
  }
}

static void enter_pointer_cb(void *data, struct wl_pointer *wl_pointer, uint32_t serial,
                             struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y) {
//	printf ("enter_pointer_cb .\n");
}

const struct wl_pointer_listener pointer_listener = {enter_pointer_cb,
                                                     do_nothing,
                                                     motion_pointer_cb,
                                                     button_pointer_cb,
                                                     do_nothing,
                                                     do_nothing,
                                                     do_nothing,
                                                     do_nothing,
                                                     do_nothing };

static void keyboard_repeat_cb(void *data, struct wl_keyboard *keyboard,
                               int32_t rate, int32_t delay) {
  (void) keyboard;
  struct keyboard *keydata = data;
  if (keydata->kb_xcb) {
    keydata->kb_xcb(WL_KEYBOARD_KEY_STATE_PRESSED,-1);
  }
}

const struct wl_keyboard_listener xkb_keyboard_listener = {keymap_format_cb,
                                                           do_nothing,
                                                           do_nothing,
                                                           key_cb,
                                                           do_nothing,
                                                           keyboard_repeat_cb };

static void touch_down_cb(void *data, struct wl_touch *wl_touch, uint32_t serial, uint32_t time,
                          struct wl_surface *surface, int32_t id, wl_fixed_t x, wl_fixed_t y) {
  struct touch *point = data;

  if (point->point_xcb) {
    point->point_xcb(WL_POINTER_BUTTON_STATE_PRESSED, BTN_LEFT,
                     wl_fixed_to_int(x),wl_fixed_to_int(y));
  }
}

static void touch_up_cb(void *data, struct wl_touch *wl_touch,
                        uint32_t serial, uint32_t time, int32_t id) {
  struct touch *point = data;
  if (point->point_xcb) {
    point->point_xcb(WL_POINTER_BUTTON_STATE_RELEASED, BTN_LEFT, -1, -1);
  }
}

static void touch_motion_cb(void *data, struct wl_touch *wl_touch, uint32_t time,
                            int32_t id, wl_fixed_t x, wl_fixed_t y) {
  struct touch *point = data;
  if (point->point_xcb) {
    point->point_xcb(POINTER_BUTTON_STATE_MOTION, 0,
                     wl_fixed_to_int(x), wl_fixed_to_int(y));
  }
}

const struct wl_touch_listener touch_listener = {touch_down_cb,
                                                 touch_up_cb,
                                                 touch_motion_cb,
                                                 do_nothing,
                                                 do_nothing,
                                                 do_nothing,
                                                 do_nothing };