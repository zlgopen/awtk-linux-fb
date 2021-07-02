/**
 * File:   egl_devices.c
 * Author: AWTK Develop Team
 * Brief:  egl devices for fsl
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

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include "fsl_egl.h"
#include "../egl_devices.h"
#include "tkc/mem.h"

typedef struct _egl_devices_fsl_context_t {
    EGLint               numconfigs;
    EGLDisplay           egldisplay;
    EGLConfig            eglconfig;
    EGLSurface           eglsurface;
    EGLContext           eglcontext;
    EGLNativeWindowType  eglNativeWindow;
    EGLNativeDisplayType eglNativeDisplayType;
} egl_devices_fsl_context_t;

static const EGLint s_configAttribs[] =
{
EGL_SAMPLES,      0,
EGL_RED_SIZE,     8,
EGL_GREEN_SIZE,   8,
EGL_BLUE_SIZE,    8,
EGL_ALPHA_SIZE,   EGL_DONT_CARE,
EGL_STENCIL_SIZE, 8,
EGL_DEPTH_SIZE,   0,
EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
EGL_MIN_SWAP_INTERVAL, 1,
EGL_NONE,
};

void* egl_devices_create(const char* filename) {
  egl_devices_fsl_context_t* ctx = TKMEM_ZALLOC(egl_devices_fsl_context_t);
  return_value_if_fail(ctx != NULL, NULL);

  ctx->eglNativeDisplayType = fsl_getNativeDisplay();
  ctx->egldisplay = eglGetDisplay(ctx->eglNativeDisplayType);
  eglInitialize(ctx->egldisplay, NULL, NULL);
  assert(eglGetError() == EGL_SUCCESS);
  eglBindAPI(EGL_OPENGL_ES_API);

  eglChooseConfig(ctx->egldisplay, s_configAttribs, &(ctx->eglconfig), 1, &(ctx->numconfigs));
  assert(eglGetError() == EGL_SUCCESS);
  assert(ctx->numconfigs == 1);

  ctx->eglNativeWindow = fsl_createwindow(ctx->egldisplay, ctx->eglNativeDisplayType, filename);	
  assert(ctx->eglNativeWindow);	

  ctx->eglsurface = eglCreateWindowSurface(ctx->egldisplay, ctx->eglconfig, ctx->eglNativeWindow, NULL);

  assert(eglGetError() == EGL_SUCCESS);
  EGLint ContextAttribList[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
  ctx->eglcontext = eglCreateContext(ctx->egldisplay, ctx->eglconfig, EGL_NO_CONTEXT, ContextAttribList );
  assert(eglGetError() == EGL_SUCCESS);
  eglMakeCurrent(ctx->egldisplay, ctx->eglsurface, ctx->eglsurface, ctx->eglcontext);
  assert(eglGetError() == EGL_SUCCESS);

  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_SCISSOR_TEST);

  eglSwapInterval(ctx->egldisplay, 1);
  assert(eglGetError() == EGL_SUCCESS);

  return (void*)ctx;
}

ret_t egl_devices_dispose(void* ctx) {
  egl_devices_fsl_context_t* context = (egl_devices_fsl_context_t*)ctx;
  return_value_if_fail(context != NULL, RET_BAD_PARAMS);

  eglMakeCurrent(context->egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  assert(eglGetError() == EGL_SUCCESS);
  eglTerminate(context->egldisplay);
  assert(eglGetError() == EGL_SUCCESS);
  eglReleaseThread();

  TKMEM_FREE(context);
  return RET_OK;
}

ret_t egl_devices_resize(void* ctx, uint32_t w, uint32_t h) {
  (void)w;
  (void)h;
  (void)ctx;
  return RET_OK;
}

lcd_linux_fb_resize_func_t egl_devices_get_default_resize_func() {
  return NULL;
}

float_t egl_devices_get_ratio(void* ctx) {
  (void)ctx;
  return 1.0f;
}

int32_t egl_devices_get_width(void* ctx) {
  EGLint width = 0;
  egl_devices_fsl_context_t* context = (egl_devices_fsl_context_t*)ctx;
  return_value_if_fail(context != NULL, RET_BAD_PARAMS);

  eglQuerySurface(context->egldisplay, context->eglsurface, EGL_WIDTH, &width);
  return (int32_t)width;
}

int32_t egl_devices_get_height(void* ctx) {
  EGLint height = 0;
  egl_devices_fsl_context_t* context = (egl_devices_fsl_context_t*)ctx;
  return_value_if_fail(context != NULL, RET_BAD_PARAMS);

  eglQuerySurface(context->egldisplay, context->eglsurface, EGL_HEIGHT, &height);
  return (int32_t)height;
}

ret_t egl_devices_make_current(void* ctx) {
  egl_devices_fsl_context_t* context = (egl_devices_fsl_context_t*)ctx;
  return_value_if_fail(context != NULL, RET_BAD_PARAMS);

  eglMakeCurrent(context->egldisplay, context->eglsurface, context->eglsurface, context->eglcontext);
  return eglGetError() == EGL_SUCCESS ? RET_OK : RET_FAIL;
}

ret_t egl_devices_swap_buffers(void* ctx) {
  egl_devices_fsl_context_t* context = (egl_devices_fsl_context_t*)ctx;
  return_value_if_fail(context != NULL, RET_BAD_PARAMS);

  eglSwapBuffers(context->egldisplay, context->eglsurface);
  return eglGetError() == EGL_SUCCESS ? RET_OK : RET_FAIL;
}
