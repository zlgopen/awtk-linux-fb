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
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include "tkc/mem.h"
#include "tkc/utils.h"
#include "../egl_devices.h"


 const static EGLint g_attr[] = {
      // some attributes to set up our egl-interface
      // EGL_BUFFER_SIZE, 32,
      // EGL_RENDERABLE_TYPE,
      // EGL_OPENGL_ES2_BIT,
      EGL_SAMPLES,
      0,
      EGL_RED_SIZE,
      8,
      EGL_GREEN_SIZE,
      8,
      EGL_BLUE_SIZE,
      8,
      EGL_ALPHA_SIZE,
      EGL_DONT_CARE,
      EGL_STENCIL_SIZE,
      8,
      EGL_DEPTH_SIZE,
      0,
      EGL_SURFACE_TYPE,
      EGL_WINDOW_BIT,
      EGL_MIN_SWAP_INTERVAL,
      0,
      EGL_NONE,
  };

typedef struct _egl_devices_x11_context_t {
  Window win;
  Display* x_display;
  EGLDisplay egl_display;
  EGLContext egl_context;
  EGLSurface egl_surface;
} egl_devices_x11_context_t;

static void DeInit_GLES(egl_devices_x11_context_t* ctx) {
  ////  cleaning up...
  eglDestroyContext(ctx->egl_display, ctx->egl_context);
  eglDestroySurface(ctx->egl_display, ctx->egl_surface);
  eglTerminate(ctx->egl_display);
  XDestroyWindow(ctx->x_display, ctx->win);
  XCloseDisplay(ctx->x_display);
}

static int Init_GLES(egl_devices_x11_context_t* ctx) {
  ///////  the X11 part  //////////////////////////////////////////////////////////////////
  // in the first part the program opens a connection to the X11 window manager
  //
  Atom atom;
  XEvent xev;
  Window root;
  Atom wm_state;
  Atom fullscreen;
  XWMHints hints;
  EGLConfig ecfg;
  EGLint num_config;
  XSetWindowAttributes swa;
  XSetWindowAttributes xattr;
  XWindowAttributes getWinAttr;
  EGLint ctxattr[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

  ctx->x_display = XOpenDisplay(NULL);  // open the standard display (the primary screen)
  if (ctx->x_display == NULL) {
    assert(0);
    return 1;
  }

  root = DefaultRootWindow(ctx->x_display);  // get the root window (usually the whole screen)
  swa.event_mask = ExposureMask | PointerMotionMask | KeyPressMask;

  XGetWindowAttributes(ctx->x_display, root, &getWinAttr);

  ctx->win = XCreateWindow(  // create a window with the provided parameters
      ctx->x_display, root, 0, 0, getWinAttr.width, getWinAttr.height, 0, CopyFromParent, InputOutput, CopyFromParent,
      CWEventMask, &swa);

  int one = 1;

  xattr.override_redirect = False;
  XChangeWindowAttributes(ctx->x_display, ctx->win, CWOverrideRedirect, &xattr);

  atom = XInternAtom(ctx->x_display, "_NET_WM_STATE_FULLSCREEN", False);
  XChangeProperty(ctx->x_display, ctx->win, XInternAtom(ctx->x_display, "_NET_WM_STATE", False),
                  XA_ATOM, 32, PropModeReplace, (unsigned char*)&atom, 1);

  XChangeProperty(ctx->x_display, ctx->win,
                  XInternAtom(ctx->x_display, "_HILDON_NON_COMPOSITED_WINDOW", False), XA_INTEGER,
                  32, PropModeReplace, (unsigned char*)&one, 1);

  hints.input = True;
  hints.flags = InputHint;
  XSetWMHints(ctx->x_display, ctx->win, &hints);

  XMapWindow(ctx->x_display, ctx->win);             // make the window visible on the screen
  XStoreName(ctx->x_display, ctx->win, "GL test");  // give the window a name

  //// get identifiers for the provided atom name strings
  wm_state = XInternAtom(ctx->x_display, "_NET_WM_STATE", False);
  fullscreen = XInternAtom(ctx->x_display, "_NET_WM_STATE_FULLSCREEN", False);

  memset(&xev, 0, sizeof(xev));

  xev.type = ClientMessage;
  xev.xclient.window = ctx->win;
  xev.xclient.message_type = wm_state;
  xev.xclient.format = 32;
  xev.xclient.data.l[0] = 1;
  xev.xclient.data.l[1] = fullscreen;
  XSendEvent(  // send an event mask to the X-server
      ctx->x_display, DefaultRootWindow(ctx->x_display), False, SubstructureNotifyMask, &xev);

  ///////  the egl part  //////////////////////////////////////////////////////////////////
  //  egl provides an interface to connect the graphics related functionality of openGL ES
  //  with the windowing interface and functionality of the native operation system (X11
  //  in our case.

  ctx->egl_display = eglGetDisplay((EGLNativeDisplayType)ctx->x_display);
  if (ctx->egl_display == EGL_NO_DISPLAY) {
    assert(0);
    return 1;
  }

  if (!eglInitialize(ctx->egl_display, NULL, NULL)) {
    assert(0);
    return 1;
  }

  eglBindAPI(EGL_OPENGL_ES_API);

  if (!eglChooseConfig(ctx->egl_display, g_attr, &ecfg, 1, &num_config)) {
    assert(0);
    return 1;
  }

  if (num_config != 1) {
    assert(0);
    return 1;
  }

  ctx->egl_surface = eglCreateWindowSurface(ctx->egl_display, ecfg, ctx->win, NULL);
  if (ctx->egl_surface == EGL_NO_SURFACE) {
    assert(0);
    return 1;
  }

  //// egl-contexts collect all state descriptions needed required for operation
  ctx->egl_context = eglCreateContext(ctx->egl_display, ecfg, EGL_NO_CONTEXT, ctxattr);
  if (ctx->egl_context == EGL_NO_CONTEXT) {
    assert(0);
    return 1;
  }

  //// associate the egl-context with the egl-surface
  eglMakeCurrent(ctx->egl_display, ctx->egl_surface, ctx->egl_surface, ctx->egl_context);

  if (!eglSwapInterval(ctx->egl_display, 0)) {
    assert(0);
    return 1;
  }
  return 0;
}

void* egl_devices_create(const char* filename) {
  egl_devices_x11_context_t* ctx = TKMEM_ZALLOC(egl_devices_x11_context_t);
  return_value_if_fail(ctx != NULL, NULL);

  (void)filename;
  Init_GLES(ctx);
  return (void*)ctx;
}

ret_t egl_devices_dispose(void* ctx) {
  egl_devices_x11_context_t* context = (egl_devices_x11_context_t*)ctx;
  return_value_if_fail(context != NULL, RET_BAD_PARAMS);
  DeInit_GLES(context);
  TKMEM_FREE(ctx);
  return RET_OK;
}

ret_t egl_devices_resize(void* ctx, uint32_t w, uint32_t h) {
  egl_devices_x11_context_t* context = (egl_devices_x11_context_t*)ctx;
  return_value_if_fail(context != NULL, RET_BAD_PARAMS);
  XResizeWindow(context->x_display, context->win, w, h);
  return RET_OK;
}

static char* lcd_linux_egl_get_screen_name(char* line_data, uint32_t line_length) {
  int32_t i = 0;
  int32_t n = 0;
  char* screen_name = NULL;
  for (; i < line_length; i++) {
    if(line_data[i] == ' ' ){
      n = i;
      break;
    }
  }
  screen_name = TKMEM_ZALLOCN(char, n + 1);
  if (screen_name != NULL) {
    memcpy(screen_name, line_data, n);
  }
  return screen_name;
}

static ret_t lcd_linux_egl_x11_resize_func(uint32_t fb_num, wh_t w, wh_t h, void* ctx) {
  bool_t is_find = FALSE;
  int32_t line_number = 0;
  char cmd[255] = {0};
  char* screen_name = NULL;
  char resize_name[20] = {0};
  char line_data[4096] = {0};
  FILE* fp = popen("xrandr", "r");
  return_value_if_fail(fp != NULL, RET_BAD_PARAMS);
  tk_snprintf(resize_name, sizeof(resize_name), "%dx%d", w, h);
  while (fgets(line_data, sizeof(line_data), fp) != NULL) {
    if (line_number == 1) {
      screen_name = lcd_linux_egl_get_screen_name(line_data, sizeof(line_data));
    } else {
      if (strstr(line_data, resize_name) != NULL) {
        is_find = TRUE;
        break;
      }
    }
    line_number++;
  }
  pclose(fp);

  return_value_if_fail(screen_name != NULL, RET_FAIL);
  // "xrandr --output HDMI-1 --mode 800x600"
  tk_snprintf(cmd, sizeof(cmd), "xrandr --output %s --mode %s", screen_name, resize_name);
  TKMEM_FREE(screen_name);

  return_value_if_fail(is_find, RET_FAIL);
  return  system(cmd) == 0 ? RET_OK : RET_FAIL;
}

lcd_linux_fb_resize_func_t egl_devices_get_default_resize_func() {
  return lcd_linux_egl_x11_resize_func;
}

float_t egl_devices_get_ratio(void* ctx) {
  (void)ctx;
  return 1.0f;
}

int32_t egl_devices_get_width(void* ctx) {
  EGLint width = 0;
  egl_devices_x11_context_t* context = (egl_devices_x11_context_t*)ctx;
  return_value_if_fail(context != NULL, RET_BAD_PARAMS);

  eglQuerySurface(context->egl_display, context->egl_surface, EGL_WIDTH, &width);
  return (int32_t)width;
}

int32_t egl_devices_get_height(void* ctx) {
  EGLint height = 0;
  egl_devices_x11_context_t* context = (egl_devices_x11_context_t*)ctx;
  return_value_if_fail(context != NULL, RET_BAD_PARAMS);

  eglQuerySurface(context->egl_display, context->egl_surface, EGL_HEIGHT, &height);
  return (int32_t)height;
}

ret_t egl_devices_make_current(void* ctx) {
  egl_devices_x11_context_t* context = (egl_devices_x11_context_t*)ctx;
  return_value_if_fail(context != NULL, RET_BAD_PARAMS);

  eglMakeCurrent(context->egl_display, context->egl_surface, context->egl_surface, context->egl_context);
  return eglGetError() == EGL_SUCCESS ? RET_OK : RET_FAIL;
}

ret_t egl_devices_swap_buffers(void* ctx) {
  egl_devices_x11_context_t* context = (egl_devices_x11_context_t*)ctx;
  return_value_if_fail(context != NULL, RET_BAD_PARAMS);

  eglSwapBuffers(context->egl_display, context->egl_surface);
  return eglGetError() == EGL_SUCCESS ? RET_OK : RET_FAIL;
}
