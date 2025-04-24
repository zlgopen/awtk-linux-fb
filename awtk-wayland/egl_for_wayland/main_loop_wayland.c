/**
 * File:   main_loop_wayland.c
 * Author: AWTK Develop Team
 * Brief:  main loop for wayland
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

#include <poll.h>
#include "GLES2/gl2.h"
#include "GLES2/gl2ext.h"
#include "EGL/egl.h"
#include "main_loop/main_loop_simple.h"
#include "native_window/native_window_fb_gl.h"
#include "tkc/thread.h"

#include "lcd_wayland.h"

static EGLint               numconfigs;
static EGLDisplay           egldisplay;
static EGLConfig            EglConfig;
static EGLSurface           eglsurface;
static EGLContext           EglContext;
static EGLNativeWindowType  eglNativeWindow;
static EGLNativeDisplayType eglNativeDisplayType;
static lcd_wayland_t *lw;

static void *wayland_run(void* ctx);

static ret_t main_loop_linux_destroy(main_loop_t* l) {
  main_loop_simple_t* loop = (main_loop_simple_t*)l;

  main_loop_simple_reset(loop);

  return RET_OK;
}

void Init_GLES(lcd_wayland_t *lw) {
  static const EGLint ContextAttributes[] = {
      EGL_CONTEXT_CLIENT_VERSION, 2,
      EGL_NONE
  };
  static const EGLint s_configAttribs[] = {
      EGL_RED_SIZE,     1,
      EGL_GREEN_SIZE,   1,
      EGL_BLUE_SIZE,    1,
      EGL_ALPHA_SIZE,   1,
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
      EGL_NONE
  };

  eglNativeDisplayType = (EGLNativeDisplayType)lw->objs.display;	// wl_display *
  egldisplay = eglGetDisplay(eglNativeDisplayType);
  eglInitialize(egldisplay, NULL, NULL);
  assert(eglGetError() == EGL_SUCCESS);
  eglBindAPI(EGL_OPENGL_ES_API);

  EGLint ConfigCount;
  EGLint ConfigNumberOfFrameBufferConfigurations;
  EGLint ConfigValue;

  eglGetConfigs(egldisplay, 0, 0, &ConfigCount);
  EGLConfig* EglAllConfigs = calloc(ConfigCount, sizeof(*EglAllConfigs));
  eglChooseConfig(egldisplay, s_configAttribs, EglAllConfigs,
                  ConfigCount, &ConfigNumberOfFrameBufferConfigurations);
  for (int i = 0; i < ConfigNumberOfFrameBufferConfigurations; ++i) {
    eglGetConfigAttrib(egldisplay, EglAllConfigs[i], EGL_BUFFER_SIZE, &ConfigValue);
    if (ConfigValue == 32) { // NOTE(Felix): Magic value from weston example
      EglConfig = EglAllConfigs[i];
      break;
    }
  }
  free(EglAllConfigs);
  EglContext = eglCreateContext(egldisplay, EglConfig, EGL_NO_CONTEXT, ContextAttributes);

  eglNativeWindow = (EGLNativeWindowType)wl_egl_window_create(lw->objs.surface, lw->objs.width, lw->objs.height);
  assert(eglNativeWindow);

  eglsurface = eglCreateWindowSurface(egldisplay, EglConfig, eglNativeWindow, NULL);
  assert(eglGetError() == EGL_SUCCESS);

  eglMakeCurrent(egldisplay, eglsurface, eglsurface, EglContext);
  assert(eglGetError() == EGL_SUCCESS);

  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_SCISSOR_TEST);

  eglSwapInterval(egldisplay, 60);
  assert(eglGetError() == EGL_SUCCESS);

}

static ret_t gles_swap_buffer(native_window_t* win) {
  eglSwapBuffers(egldisplay, eglsurface);
  return RET_OK;
}

static ret_t gles_make_current(native_window_t* win) {
  EGLint width = 0;
  EGLint height = 0;
  eglQuerySurface(egldisplay, eglsurface, EGL_WIDTH, &width);
  eglQuerySurface(egldisplay, eglsurface, EGL_HEIGHT, &height);

  eglMakeCurrent(egldisplay, eglsurface, eglsurface, EglContext);
  assert(eglGetError() == EGL_SUCCESS);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  glViewport(0, 0, width, height);
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f );

  return RET_OK;
}

static ret_t gles_destroy(native_window_t* win) {
  eglDestroyContext(egldisplay, EglContext);
  eglDestroySurface(egldisplay, eglsurface);
  eglTerminate(egldisplay);
  if (lw) {
    destroy_wayland_data (&lw->objs);
  }
}

main_loop_t* main_loop_init(int w, int h) {
  lw = lcd_wayland_create(w, h);

  return_value_if_fail(lw != NULL, NULL);

  Init_GLES(lw);

  eglQuerySurface(egldisplay, eglsurface, EGL_WIDTH, &w);
  eglQuerySurface(egldisplay, eglsurface, EGL_HEIGHT, &h);

  native_window_t* win = native_window_fb_gl_init(w, h, 1.0);

  native_window_fb_gl_set_swap_buffer_func(win, gles_swap_buffer);
  native_window_fb_gl_set_make_current_func(win, gles_make_current);
  native_window_fb_gl_set_destroy_func(win, gles_destroy);

  main_loop_simple_t *loop = main_loop_simple_init(w, h, NULL, NULL);

  loop->base.destroy = main_loop_linux_destroy;

  tk_thread_t* thread = tk_thread_create(wayland_run, lw);
  if (thread != NULL) {
    tk_thread_start(thread);
  }

  return (main_loop_t*)loop;
}

static void PlatformPollEvents(wayland_data_t* objs) {
  struct wl_display* display = objs->display;
  struct pollfd fds[] = {
      { wl_display_get_fd(display), POLLIN },
  };

  while (wl_display_prepare_read(display) != 0) {
    wl_display_dispatch_pending(display);
  }

  wl_display_flush(display);

  if (poll(fds, 1, -1) > 0) {
    wl_display_read_events(display);
    wl_display_dispatch_pending(display);
  } else {
    wl_display_cancel_read(display);
  }
  kb_repeat();
}

static void* wayland_run(void* ctx) {
  lcd_wayland_t* lw = (lcd_wayland_t*)ctx;

  while(1) {
    PlatformPollEvents(&lw->objs);
  }
}

