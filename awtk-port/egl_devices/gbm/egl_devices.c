/**
 * File:   egl_devices.c
 * Author: AWTK Develop Team
 * Brief:  egl devices for gbm
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

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>

typedef struct _egl_devices_gbm_context_t {
  uint32_t screen_width;
  uint32_t screen_height;

  int device;
  uint32_t connector_id;
  drmModeModeInfo mode_info;
  drmModeCrtc *crtc;

  uint32_t mode_list_size;
  drmModeModeInfo* mode_list;

  struct gbm_device *gbm_device;
  struct gbm_surface *gbm_surface;

  EGLDisplay egl_display;
  EGLSurface egl_surface;
  EGLContext egl_context;
  int egl_config_index;
  
  EGLConfig *egl_configs;

  struct gbm_bo *previous_bo;
  uint32_t previous_fb;
} egl_devices_gbm_context_t;

const EGLint attribute_list[] = {
  EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
  EGL_RED_SIZE, 8,
  EGL_GREEN_SIZE, 8,
  EGL_BLUE_SIZE, 8,
  EGL_ALPHA_SIZE, 0,
  EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
  EGL_NONE
};

const EGLint context_attrib_list[] = {
  EGL_CONTEXT_CLIENT_VERSION, 2,
  EGL_NONE
};

static drmModeConnector *find_connector (int device, drmModeRes *resources) {
  for (int i=0; i<resources->count_connectors; i++) {
    drmModeConnector *connector = drmModeGetConnector (device, resources->connectors[i]);
    if (connector->connection == DRM_MODE_CONNECTED) {
      return connector;
    }
    drmModeFreeConnector (connector);
  }
  return NULL; // if no connector found
}

static drmModeEncoder *find_encoder (int device, drmModeRes *resources, drmModeConnector *connector) {
  if (connector->encoder_id) {
    return drmModeGetEncoder (device, connector->encoder_id);
  }
  return NULL; // if no encoder found
}

static int match_config_to_visual(EGLDisplay egl_display, EGLint visual_id, EGLConfig *configs, int count) {
  EGLint id = 0;
  for (int i = 0; i < count; ++i) {
    if (!eglGetConfigAttrib(egl_display, configs[i], EGL_NATIVE_VISUAL_ID,&id)) continue;
    if (id == visual_id) return i;
  }
  return -1;
}

static void swap_buffers (egl_devices_gbm_context_t* context) {
  struct gbm_bo *bo = NULL;	
  uint32_t handle = 0;
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t pitch = 0;
  uint32_t fb = 0;

  eglSwapBuffers (context->egl_display, context->egl_surface);
  bo = gbm_surface_lock_front_buffer (context->gbm_surface);
  handle = gbm_bo_get_handle (bo).u32;
  pitch = gbm_bo_get_stride (bo);
  width = gbm_bo_get_width(bo);
  height = gbm_bo_get_height(bo);
  drmModeAddFB (context->device, width, height, 24, 32, pitch, handle, &fb);
  drmModeSetCrtc (context->device, context->crtc->crtc_id, fb, 0, 0, &context->connector_id, 1, &context->mode_info);
  if (context->previous_bo) {
    drmModeRmFB (context->device, context->previous_fb);
    gbm_surface_release_buffer (context->gbm_surface, context->previous_bo);
  }
  context->previous_bo = bo;
  context->previous_fb = fb;
}

static void init_ogl(egl_devices_gbm_context_t *ctx) {
  uint32_t i = 0;
  drmModeRes *resources = NULL;
  drmModeConnector *connector = NULL;
  drmModeEncoder *encoder = NULL;

  EGLBoolean result = 0;
  EGLint egl_num_config = 0;
  EGLint egl_count=0;

  ctx->device = open ("/dev/dri/by-path/platform-gpu-card", O_RDWR);
  if (ctx->device < 0) {
    ctx->device = open ("/dev/dri/card0", O_RDWR);
  }
  resources = drmModeGetResources (ctx->device);
  connector = find_connector (ctx->device, resources);
  ctx->connector_id = connector->connector_id;
  ctx->mode_info = connector->modes[0];
  encoder = find_encoder (ctx->device, resources, connector);
  ctx->crtc = drmModeGetCrtc (ctx->device, encoder->crtc_id);

  ctx->mode_list = TKMEM_ZALLOCN(drmModeModeInfo, connector->count_modes);
  ctx->mode_list_size = connector->count_modes;
  for (; i < connector->count_modes; i++) {
    memcpy(&(ctx->mode_list[i]), &connector->modes[i], sizeof(drmModeModeInfo));
  }

  drmModeFreeEncoder (encoder);
  drmModeFreeConnector (connector);
  drmModeFreeResources (resources);
  ctx->gbm_device = gbm_create_device (ctx->device);
  ctx->gbm_surface = gbm_surface_create (ctx->gbm_device, ctx->mode_info.hdisplay, ctx->mode_info.vdisplay, GBM_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT|GBM_BO_USE_RENDERING);
  ctx->egl_display = eglGetDisplay ((EGLNativeDisplayType)ctx->gbm_device);
  ctx->screen_width = ctx->mode_info.hdisplay;
  ctx->screen_height = ctx->mode_info.vdisplay;
  eglInitialize (ctx->egl_display, NULL ,NULL);
  eglBindAPI (EGL_OPENGL_API);
  eglGetConfigs(ctx->egl_display, NULL, 0, &egl_count);
  ctx->egl_configs = (EGLConfig *)TKMEM_ALLOC(egl_count * sizeof(EGLConfig));
  eglChooseConfig (ctx->egl_display, attribute_list, ctx->egl_configs, egl_count, &egl_num_config);
  ctx->egl_config_index = match_config_to_visual(ctx->egl_display,GBM_FORMAT_XRGB8888,ctx->egl_configs,egl_num_config);
  ctx->egl_context = eglCreateContext (ctx->egl_display, ctx->egl_configs[ctx->egl_config_index], EGL_NO_CONTEXT, context_attrib_list);

  ctx->egl_surface = eglCreateWindowSurface (ctx->egl_display, ctx->egl_configs[ctx->egl_config_index], (EGLNativeWindowType)ctx->gbm_surface, NULL);
  assert(ctx->egl_surface != EGL_NO_SURFACE);

  result = eglMakeCurrent(ctx->egl_display, ctx->egl_surface, ctx->egl_surface, ctx->egl_context);
  assert(EGL_FALSE != result);
  
}

ret_t egl_devices_resize(void* ctx, uint32_t w, uint32_t h) {
  int32_t i = 0;
  int32_t find_number = -1;
  egl_devices_gbm_context_t* context = (egl_devices_gbm_context_t*)ctx;
  return_value_if_fail(context != NULL, RET_BAD_PARAMS);

  for (; i < context->mode_list_size; i++) {
    if (context->mode_list[i].hdisplay == w && context->mode_list[i].vdisplay == h) {
      find_number = i;
      break;
    }
  }
  return_value_if_fail(find_number >= 0, RET_NOT_FOUND);

  eglDestroySurface(context->egl_display, context->egl_surface);
  gbm_surface_destroy(context->gbm_surface);

  memcpy(&(context->mode_info), &(context->mode_list[find_number]), sizeof(drmModeModeInfo));
  context->gbm_surface = gbm_surface_create(context->gbm_device, context->mode_info.hdisplay, context->mode_info.vdisplay, GBM_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT|GBM_BO_USE_RENDERING);
  context->egl_surface = eglCreateWindowSurface(context->egl_display, context->egl_configs[context->egl_config_index], (EGLNativeWindowType)context->gbm_surface, NULL);
  assert(context->egl_surface != EGL_NO_SURFACE);
  return RET_OK;
}

static ret_t lcd_linux_egl_gbm_resize_func(uint32_t fb_num, wh_t w, wh_t h, void* ctx) {
  return RET_OK;
}

lcd_linux_fb_resize_func_t egl_devices_get_default_resize_func() {
  return lcd_linux_egl_gbm_resize_func;
}

void* egl_devices_create(const char* filename) {
  egl_devices_gbm_context_t* ctx = TKMEM_ZALLOC(egl_devices_gbm_context_t);
  return_value_if_fail(ctx != NULL, NULL);
  (void)filename;

  init_ogl(ctx);

  printf("EGL Information:\n");
  printf("\tVendor:\t\t%s\n", eglQueryString(ctx->egl_display, EGL_VENDOR));
  printf("\tVersion:\t%s\n", eglQueryString(ctx->egl_display, EGL_VERSION));
  printf("\tClient APIs:\t%s\n", eglQueryString(ctx->egl_display, EGL_CLIENT_APIS));
  printf("\tExtensions:\t%s\n", eglQueryString(ctx->egl_display, EGL_EXTENSIONS));
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
  egl_devices_gbm_context_t* context = (egl_devices_gbm_context_t*)ctx;
  return_value_if_fail(context != NULL, RET_BAD_PARAMS);

  drmModeSetCrtc (context->device, context->crtc->crtc_id, context->crtc->buffer_id, 
    context->crtc->x, context->crtc->y, &context->connector_id, 1, &context->crtc->mode);
  drmModeFreeCrtc (context->crtc);

  if (context->previous_bo) {
    drmModeRmFB (context->device, context->previous_fb);
    gbm_surface_release_buffer (context->gbm_surface, context->previous_bo);
  }

  eglDestroySurface (context->egl_display, context->egl_surface);
  gbm_surface_destroy (context->gbm_surface);
  eglDestroyContext (context->egl_display, context->egl_context);
  eglTerminate (context->egl_display);
  gbm_device_destroy (context->gbm_device);

  close (context->device);
  
  TKMEM_FREE(context->egl_configs);
  TKMEM_FREE(context->mode_list);
  TKMEM_FREE(context);
  return RET_OK;
}

float_t egl_devices_get_ratio(void* ctx) {
  (void)ctx;
  return 1.0f;
}

int32_t egl_devices_get_width(void* ctx) {
  EGLint width = 0;
  egl_devices_gbm_context_t* context = (egl_devices_gbm_context_t*)ctx;
  return_value_if_fail(context != NULL, RET_BAD_PARAMS);

  eglQuerySurface(context->egl_display, context->egl_surface, EGL_WIDTH, &width);
  return (int32_t)width;
}

int32_t egl_devices_get_height(void* ctx) {
  EGLint height = 0;
  egl_devices_gbm_context_t* context = (egl_devices_gbm_context_t*)ctx;
  return_value_if_fail(context != NULL, RET_BAD_PARAMS);

  eglQuerySurface(context->egl_display, context->egl_surface, EGL_HEIGHT, &height);
  return (int32_t)height;
}

ret_t egl_devices_make_current(void* ctx) {
  egl_devices_gbm_context_t* context = (egl_devices_gbm_context_t*)ctx;
  return_value_if_fail(context != NULL, RET_BAD_PARAMS);

  eglMakeCurrent(context->egl_display, context->egl_surface, context->egl_surface, context->egl_context);
  return eglGetError() == EGL_SUCCESS ? RET_OK : RET_FAIL;
}

ret_t egl_devices_swap_buffers(void* ctx) {
  egl_devices_gbm_context_t* context = (egl_devices_gbm_context_t*)ctx;
  return_value_if_fail(context != NULL, RET_BAD_PARAMS);

  swap_buffers(context);
  return eglGetError() == EGL_SUCCESS ? RET_OK : RET_FAIL;
}