/**
 * File:   lcd_wayland.c
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
#include "lcd_wayland.h"

#include "base/idle.h"
#include "base/timer.h"
#include "base/window_manager.h"
#include "main_loop/main_loop_simple.h"
#include "tkc/thread.h"
#include "base/lcd_fb_dirty_rects.h"

enum key_repeat_state {
  repeat_key_released = 0,
  repeat_key_pressed = 10,
  repeat_key_delay,
  repeat_key_rate,
};

lcd_wayland_t *lw = NULL;
enum key_repeat_state __repeat_state = repeat_key_released;
static int key_value;

void on_app_exit(void) {
  if (lw) {
    destroy_wayland_data (&lw->objs);
  }
}

static ret_t wayland_flush(lcd_t* lcd) {
  lcd_wayland_t *lw = lcd->impl_data;
  ref_display(lw->objs.surface,lw->current->wl_buffer,lcd->w,lcd->h);
  wl_display_flush(lw->objs.display);
  return RET_OK;
}

static ret_t wayland_sync(lcd_t* lcd) {
  lcd_wayland_t *lw = lcd->impl_data;
  struct buffer *buf = lw->impl_data;
  ThreadSignal_Wait(&buf->used); //  todo 测试 垂直同步
  return RET_OK;
}

static ret_t input_dispatch_to_main_loop(void* ctx, const event_queue_req_t* e) {
  main_loop_queue_event((main_loop_t*)ctx, e);
  return RET_OK;
}

extern int32_t map_key(uint8_t code);
static void key_input_dispatch(int state,int key) {
  event_queue_req_t req;

  req.event.type = (state == WL_KEYBOARD_KEY_STATE_PRESSED) ? EVT_KEY_DOWN : EVT_KEY_UP;
  req.key_event.key = map_key(key);

  input_dispatch_to_main_loop(main_loop(), &(req));

  req.event.type = EVT_NONE;

  if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
    __repeat_state = repeat_key_pressed;
    key_value = key;
  } else {
    __repeat_state = repeat_key_released;
  }
}

static void mouse_point_dispatch(int state,int button, int x,int y) {
  event_queue_req_t r;
  event_queue_req_t* req = &r;
  main_loop_simple_t* l = (main_loop_simple_t*)main_loop();
  static int __x,__y;

  if (x > 0) {
    __x = x;
  }

  if (y > 0) {
    __y = y;
  }

  req->pointer_event.x = __x;
  req->pointer_event.y = __y;
  req->event.time = time_now_ms();

  if (button == BTN_LEFT) {
    req->event.size = sizeof(req->pointer_event);
    switch (state) {
      case WL_POINTER_BUTTON_STATE_PRESSED:
        req->event.type = EVT_POINTER_DOWN;
        l->pressed = TRUE;
        req->pointer_event.pressed = TRUE;
        break;
      case WL_POINTER_BUTTON_STATE_RELEASED:
        req->event.type = EVT_POINTER_UP;
        req->pointer_event.pressed = l->pressed;
        l->pressed = FALSE;
        break;
    }
  } else if (button == BTN_RIGHT) {
    req->event.size = sizeof(req->pointer_event);
    if (state == WL_POINTER_BUTTON_STATE_RELEASED) {
      req->event.type = EVT_CONTEXT_MENU;
      req->pointer_event.pressed = FALSE;
    }
  } else if (button == BTN_MIDDLE) {
    req->event.size = sizeof(req->key_event);
    switch (state) {
      case WL_POINTER_BUTTON_STATE_PRESSED:
        req->event.type = EVT_KEY_DOWN;
        req->key_event.key = TK_KEY_WHEEL;
        break;
      case WL_POINTER_BUTTON_STATE_RELEASED:
        req->event.type = EVT_KEY_UP;
        req->key_event.key = TK_KEY_WHEEL;
        break;
    }
  } else {
    req->event.size = sizeof(req->pointer_event);
    req->event.type = EVT_POINTER_MOVE;
  }

  input_dispatch_to_main_loop(main_loop(), req);
}

static lcd_t* lcd_linux_create_flushable(lcd_wayland_t *lw, int w, int h) {
  struct wayland_data *objs = &lw->objs;
  struct wayland_output *out = container_of ( objs->monitors->next,
                                              struct wayland_output,
                                              link);
  size_t width = w;
  size_t height = h;

  if(out->info.transform == WL_OUTPUT_TRANSFORM_90 || out->info.transform == WL_OUTPUT_TRANSFORM_270) {
    height = w;
    width = w;
  }

  int line_length = width * 4;

  struct buffer *buffer = wayland_create_double_buffer(objs->shm,width,height);

  uint8_t* online_fb = (void*)buffer->bufs->pixels;

  lw->current = buffer->bufs;
  lw->impl_data = buffer;

  lcd_t *lcd = lcd_mem_bgra8888_create_single_fb(width, height, online_fb);

  if(lcd != NULL) {
    lcd->impl_data = lw;
    lcd->sync = wayland_sync;
    lcd->flush = wayland_flush;
    lcd_mem_set_line_length(lcd, line_length);
  }

  lw->objs.inputs.keyboard.kb_xcb = key_input_dispatch;
  lw->objs.inputs.mouse.point_xcb = mouse_point_dispatch;
  lw->objs.inputs.touch.point_xcb = mouse_point_dispatch;

  atexit(on_app_exit);

  return lcd;
}

// 为了实现按下重复发送 修改写法（可能出现抬起后又发送按下的事件）
void kb_repeat(struct wayland_data *objs) {
  static uint32_t repeat_count = 0;
  switch(__repeat_state){
    case repeat_key_pressed:
      repeat_count = 0;
      __repeat_state = repeat_key_delay;
      break;
    case repeat_key_delay:
      repeat_count ++;
      if(repeat_count >= 20){
        repeat_count = 0;
        __repeat_state = repeat_key_rate;
      }
      break;
    case repeat_key_rate:
      repeat_count ++;
      if((repeat_count % 2) == 0){
        event_queue_req_t req;

        req.event.type = EVT_KEY_DOWN;
        req.key_event.key = map_key(key_value);
        input_dispatch_to_main_loop(main_loop(), &(req));
      }
      break;
    default:
    case repeat_key_released:
      break;
  }
}

lcd_t *lcd_wayland_create(int w, int h) {
  lw = calloc(1,sizeof(lcd_wayland_t));
  if (lw && setup_wayland (&lw->objs,0) != SETUP_OK) {
    destroy_wayland_data (&lw->objs);
    return NULL;
  }

  return lcd_linux_create_flushable(lw, w, h);
}


