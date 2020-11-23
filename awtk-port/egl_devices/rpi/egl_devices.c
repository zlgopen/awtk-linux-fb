/**
 * File:   egl_devices.c
 * Author: AWTK Develop Team
 * Brief:  egl devices for x11
 *
 * Copyright (c) 2020 - 2020  Guangzhou ZHIYUAN Electronics Co.,Ltd.
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
 * 2020-11-06 Lou ZhiMing <luozhiming@zlg.com> created
 *
 */

#include <signal.h>
#include <stdio.h>
#include <assert.h>
#include <sys/time.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "tkc/mem.h"
#include "../egl_devices.h"
#define bool_t 1
#include "bcm_host.h"

typedef struct _egl_devices_rpi_context_t {
  uint32_t screen_width;
  uint32_t screen_height;
  EGLDisplay display;
  EGLSurface surface;
  EGLContext context;
} egl_devices_rpi_context_t;

const EGLint attribute_list[] = {
  EGL_RED_SIZE, 8,
  EGL_GREEN_SIZE, 8,
  EGL_BLUE_SIZE, 8,
  EGL_ALPHA_SIZE, 8,
  EGL_DEPTH_SIZE, 8,
  EGL_STENCIL_SIZE, 8,
  EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
  EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
  EGL_NONE
};

const EGLint context_attrib_list[] = {
  EGL_CONTEXT_CLIENT_VERSION, 2,
  EGL_NONE
};

static void init_ogl(egl_devices_rpi_context_t *ctx) {
  int32_t success = 0;
  EGLBoolean result;
  EGLint num_config;
  VC_RECT_T dst_rect;
  VC_RECT_T src_rect;

  EGLConfig config;
  DISPMANX_UPDATE_HANDLE_T dispman_update;
  DISPMANX_ELEMENT_HANDLE_T dispman_element;
  DISPMANX_DISPLAY_HANDLE_T dispman_display;
  static EGL_DISPMANX_WINDOW_T nativewindow;


  ctx->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  assert(ctx->display!=EGL_NO_DISPLAY);

  result = eglInitialize(ctx->display, NULL, NULL);
  assert(EGL_FALSE != result);

  result = eglChooseConfig(ctx->display, attribute_list, &config, 1, &num_config);
  assert(EGL_FALSE != result);

  result = eglBindAPI(EGL_OPENGL_ES_API);
  assert(EGL_FALSE != result);

  ctx->context = eglCreateContext(ctx->display, config, EGL_NO_CONTEXT, context_attrib_list);
  assert(ctx->context!=EGL_NO_CONTEXT);

  success = graphics_get_display_size(0, &ctx->screen_width, &ctx->screen_height);
  assert( success >= 0 );

  dst_rect.x = 0;
  dst_rect.y = 0;
  dst_rect.width = ctx->screen_width;
  dst_rect.height = ctx->screen_height;

  src_rect.x = 0;
  src_rect.y = 0;
  src_rect.width = ctx->screen_width << 16;
  src_rect.height = ctx->screen_height << 16;

  dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
  dispman_update = vc_dispmanx_update_start( 0 );

  dispman_element = vc_dispmanx_element_add ( dispman_update, dispman_display, 0, &dst_rect, 0, &src_rect, DISPMANX_PROTECTION_NONE, 0, 0, DISPMANX_NO_ROTATE);

  nativewindow.element = dispman_element;
  nativewindow.width = ctx->screen_width;
  nativewindow.height = ctx->screen_height;
  vc_dispmanx_update_submit_sync( dispman_update );

  ctx->surface = eglCreateWindowSurface(ctx->display, config, &nativewindow, NULL );
  assert(ctx->surface != EGL_NO_SURFACE);

  result = eglMakeCurrent(ctx->display, ctx->surface, ctx->surface, ctx->context);
  assert(EGL_FALSE != result);
}

void* egl_devices_create(const char* filename) {
  egl_devices_rpi_context_t* ctx = TKMEM_ZALLOC(egl_devices_rpi_context_t);
  return_value_if_fail(ctx != NULL, NULL);
  (void)filename;

  bcm_host_init();
  init_ogl(ctx);

  printf("EGL Information:\n");
  printf("\tVendor:\t\t%s\n", eglQueryString(ctx->display, EGL_VENDOR));
  printf("\tVersion:\t%s\n", eglQueryString(ctx->display, EGL_VERSION));
  printf("\tClient APIs:\t%s\n", eglQueryString(ctx->display, EGL_CLIENT_APIS));
  printf("\tExtensions:\t%s\n", eglQueryString(ctx->display, EGL_EXTENSIONS));
  printf("\n");
  printf("OpenGL Information:\n");
  printf("\tVendor:\t\t%s\n", glGetString(GL_VENDOR));
  printf("\tRenderer:\t%s\n", glGetString(GL_RENDERER));
  printf("\tVersion:\t%s\n", glGetString(GL_VERSION));
  printf("\tExtensions:\t%s\n", glGetString(GL_EXTENSIONS));
  printf("\n");

  glViewport(0, 0, (GLsizei) ctx->screen_width, (GLsizei) ctx->screen_height);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClearStencil(0);
  glStencilMask(0xffffffff);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  return (void*)ctx;
}

ret_t egl_devices_dispose(void* ctx) {
  egl_devices_rpi_context_t* context = (egl_devices_rpi_context_t*)ctx;
  return_value_if_fail(context != NULL, RET_BAD_PARAMS);

  eglMakeCurrent(context->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  eglDestroySurface(context->display, context->surface);
  eglDestroyContext(context->display, context->context);
  eglTerminate(context->display );
  bcm_host_deinit();
  return RET_OK;
}

float_t egl_devices_get_ratio(void* ctx) {
  (void)ctx;
  return 1.0f;
}

int32_t egl_devices_get_width(void* ctx) {
  EGLint width = 0;
  egl_devices_rpi_context_t* context = (egl_devices_rpi_context_t*)ctx;
  return_value_if_fail(context != NULL, RET_BAD_PARAMS);

  eglQuerySurface(context->display, context->surface, EGL_WIDTH, &width);
  return (int32_t)width;
}

int32_t egl_devices_get_height(void* ctx) {
  EGLint height = 0;
  egl_devices_rpi_context_t* context = (egl_devices_rpi_context_t*)ctx;
  return_value_if_fail(context != NULL, RET_BAD_PARAMS);

  eglQuerySurface(context->display, context->surface, EGL_HEIGHT, &height);
  return (int32_t)height;
}

ret_t egl_devices_make_current(void* ctx) {
  egl_devices_rpi_context_t* context = (egl_devices_rpi_context_t*)ctx;
  return_value_if_fail(context != NULL, RET_BAD_PARAMS);

  eglMakeCurrent(context->display, context->surface, context->surface, context->context);
  return eglGetError() == EGL_SUCCESS ? RET_OK : RET_FAIL;
}

ret_t egl_devices_swap_buffers(void* ctx) {
  egl_devices_rpi_context_t* context = (egl_devices_rpi_context_t*)ctx;
  return_value_if_fail(context != NULL, RET_BAD_PARAMS);

  eglSwapBuffers(context->display, context->surface);
  return eglGetError() == EGL_SUCCESS ? RET_OK : RET_FAIL;
}