/**
 * File:   wayland_tools.c
 * Author: AWTK Develop Team
 * Brief:  wayland tools
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

static void output_mode_cb(void* data, struct wl_output* out, uint32_t flags,
                           int32_t width, int32_t height, int32_t refresh_rate) {
  output_info_t* info = data;
  if (flags & WL_OUTPUT_MODE_CURRENT) {
    info->width = width;
    info->height = height;
    info->hertz = refresh_rate;
  }
}

static void output_geometry_cb(void* data, struct wl_output* wl_output,
                               int32_t x, int32_t y, int32_t physical_width, int32_t physical_height,
                               int32_t subpixel, const char* make, const char* model,
                               int32_t transform) {
  output_info_t* info = data;
  info->transform = transform;
}

static const struct wl_output_listener output_listener = {
    output_geometry_cb,
    output_mode_cb,
    do_nothing,
    do_nothing
};

static void shell_ping_respond(void* data, struct xdg_wm_base* shell,
                               uint32_t serial) {
  (void) data;
  xdg_wm_base_pong(shell, serial);
}

static const struct xdg_wm_base_listener xdg_shell_listener = { 
    shell_ping_respond
};

static const struct zwp_fullscreen_shell_v1_listener fullscreen_shell_listener = {
  do_nothing
};

extern const struct wl_keyboard_listener xkb_keyboard_listener;
extern const struct wl_pointer_listener pointer_listener;
extern const struct wl_touch_listener touch_listerner;

static void calc_capabilities(void* data, struct wl_seat* s, uint32_t cap) {
  wayland_data_t* objs = data;
  input_bundle_t* inputs = &(objs->inputs);
  (void) s;
  //Cap is a bitfield. The WL_SEAT_CAPABILITY_XXX enum is a mask
  // that selects the corresponding bit.
  inputs->seat = s;
  if (cap & WL_SEAT_CAPABILITY_KEYBOARD) {
    if (inputs->keyboard.kbd == NULL) {
      inputs->keyboard.kbd = wl_seat_get_keyboard(s);
      wl_keyboard_add_listener(inputs->keyboard.kbd, &xkb_keyboard_listener,
                               &inputs->keyboard);
    }
  }
  if (cap & WL_SEAT_CAPABILITY_POINTER) {
    inputs->mouse.pointer = wl_seat_get_pointer(s);
    if (inputs->mouse.pointer) {
      inputs->mouse.point_surface = wl_compositor_create_surface(objs->compositor);
      wl_pointer_add_listener(inputs->mouse.pointer, &pointer_listener, inputs);
    }
  } else {
    if (inputs->mouse.pointer) {
      wl_surface_destroy(inputs->mouse.point_surface);
      wl_pointer_release(inputs->mouse.pointer);
      inputs->mouse.pointer = NULL;
    }
  }
  if (cap & WL_SEAT_CAPABILITY_TOUCH) {
    inputs->touch.pointer = wl_seat_get_touch(s);
    if (inputs->touch.pointer) {
      wl_touch_add_listener(inputs->touch.pointer, &touch_listerner, &inputs->touch);
    } else {
      wl_touch_release(inputs->touch.pointer);
    }
  }
}

static const struct wl_seat_listener seat_listener = {
    calc_capabilities,
    do_nothing,
};

static void global_registry_handler(void* data, struct wl_registry* registry, uint32_t id,
                                    const char* interface, uint32_t version) {
  wayland_data_t* objs = data;
  if (strcmp(interface, wl_compositor_interface.name) == 0) {
    objs->compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 4);
    objs->surface = wl_compositor_create_surface(objs->compositor);
  } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
    objs->xdg_wm_base = wl_registry_bind(registry, id, &xdg_wm_base_interface, 1);
    xdg_wm_base_add_listener(objs->xdg_wm_base, &xdg_shell_listener, NULL);
  } else if (strcmp(interface, zwp_fullscreen_shell_v1_interface.name) == 0) {
    objs->fullscreen_shell = wl_registry_bind(registry, id, &zwp_fullscreen_shell_v1_interface, 1);
    zwp_fullscreen_shell_v1_add_listener(objs->fullscreen_shell, &fullscreen_shell_listener, NULL);
  } else if (strcmp(interface, wl_shm_interface.name) == 0) {
    objs->shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
  } else if (strcmp(interface, wl_seat_interface.name) == 0) {
    objs->inputs.seat = wl_registry_bind(registry, id, &wl_seat_interface, 5);
    wl_seat_add_listener(objs->inputs.seat, &seat_listener, objs);
  } else if (strcmp(interface, wl_output_interface.name) == 0) {
    struct wl_output* wl_out = wl_registry_bind(registry, id, &wl_output_interface, 2);
    wayland_output_t* ua_out = malloc(sizeof(wayland_output_t));
    ua_out->out = wl_out;
    wl_output_add_listener(ua_out->out, &output_listener, &ua_out->info);
    wl_list_insert(objs->monitors, &ua_out->link);
  }
}

static const struct wl_registry_listener registry_listener = {
    global_registry_handler,
    do_nothing,
};

static void surface_configure_cb(void* data, struct xdg_surface* surface,
                                 uint32_t serial) {
  (void) data;
  xdg_surface_ack_configure(surface, serial);
}

static const struct xdg_surface_listener xdg_surface_listener = {
    surface_configure_cb,
};

static const struct xdg_toplevel_listener xdg_top_listener = {
    do_nothing,
    do_nothing
};

#define RETURN_WITH(s)	{printf("return with " #s "\n");return s;}

WAYLAND_SETUP_ERR setup_wayland(wayland_data_t* objs, bool_t fullscreen) {
  memset(objs, 0, sizeof(wayland_data_t));
  objs->monitors = malloc(sizeof(struct wl_list));
  wl_list_init(objs->monitors);

  memset(&objs->inputs, 0, sizeof(input_bundle_t));
  objs->inputs.keyboard.ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

  objs->display = wl_display_connect(NULL);
  if (objs->display == NULL)
    RETURN_WITH (NO_WAY_DISP);

  objs->registry = wl_display_get_registry(objs->display);
  if (objs->registry == NULL)
    RETURN_WITH (NO_REG);
  wl_registry_add_listener(objs->registry, &registry_listener, objs);
  wl_display_roundtrip(objs->display); // Wait for registry listener to run
  if (objs->compositor == NULL)
    RETURN_WITH (NO_COMP);
  if (objs->xdg_wm_base == NULL && objs->fullscreen_shell == NULL)
    RETURN_WITH (NO_SHELL);
  if (objs->inputs.seat == NULL)
    RETURN_WITH (NO_SEAT);
  if (objs->shm == NULL)
    RETURN_WITH (NO_SHM);
  objs->inputs.mouse.cursor_theme = wl_cursor_theme_load(NULL, 1, objs->shm);
  if (wl_list_empty(objs->monitors))
    RETURN_WITH (NO_MONITORS);
  if (objs->surface == NULL)
    RETURN_WITH (NO_SURFACE);

  if (objs->xdg_wm_base != NULL) {
    objs->xdg_surface = xdg_wm_base_get_xdg_surface(objs->xdg_wm_base, objs->surface);
    if (objs->xdg_surface == NULL)
      RETURN_WITH (NO_SHELL_SURFACE);
  
    xdg_surface_add_listener(objs->xdg_surface, &xdg_surface_listener, NULL);
  
    objs->xdg_toplevel = xdg_surface_get_toplevel(objs->xdg_surface);
    if (objs->xdg_toplevel == NULL)
      RETURN_WITH (NO_TOPLEVEL);
  
    if (fullscreen) {
      xdg_toplevel_set_minimized(objs->xdg_toplevel);
      xdg_toplevel_set_title(objs->xdg_toplevel, "Unknown Animal");
      wl_display_roundtrip(objs->display);
  
      wayland_output_t* out = container_of(objs->monitors->next,
                                           wayland_output_t, link);
  
      xdg_toplevel_set_fullscreen(objs->xdg_toplevel, out->out);
      xdg_toplevel_add_listener(objs->xdg_toplevel, &xdg_top_listener, NULL);
    }
  } else if (objs->fullscreen_shell != NULL) {
    zwp_fullscreen_shell_v1_present_surface(objs->fullscreen_shell, objs->surface, 
                                            ZWP_FULLSCREEN_SHELL_V1_PRESENT_METHOD_DEFAULT, NULL);
  }
  
  wl_surface_commit(objs->surface);

  wl_display_roundtrip(objs->display);

  return SETUP_OK;
}

void destroy_wayland_data(wayland_data_t* objs) {
  wayland_output_t* out = NULL;
  struct wl_list* head = objs->monitors;
  struct wl_list* current = head->next;

  while (current != head) {
    struct wl_list* next = current->next;
    out = container_of(current, wayland_output_t, link);
    wl_output_destroy(out->out);
    free(out);
    current = next;
  }
  free(head);
  xkb_keymap_unref(objs->inputs.keyboard.map);
  xkb_state_unref(objs->inputs.keyboard.kb_state);
  xkb_context_unref(objs->inputs.keyboard.ctx);
  if (objs->inputs.keyboard.kbd)
    wl_keyboard_destroy(objs->inputs.keyboard.kbd);
  if (objs->inputs.mouse.pointer)
    wl_pointer_destroy(objs->inputs.mouse.pointer);
  if (objs->inputs.touch.pointer)
	  wl_touch_destroy(objs->inputs.touch.pointer);
  if (objs->inputs.seat)
    wl_seat_destroy(objs->inputs.seat);
  if (objs->shm)
    wl_shm_destroy(objs->shm);
  if (objs->xdg_toplevel)
    xdg_toplevel_destroy(objs->xdg_toplevel);
  if (objs->xdg_surface)
    xdg_surface_destroy(objs->xdg_surface);
  if (objs->xdg_wm_base)
    xdg_wm_base_destroy(objs->xdg_wm_base);
  if (objs->fullscreen_shell)
    zwp_fullscreen_shell_v1_destroy(objs->fullscreen_shell);
  if (objs->surface)
    wl_surface_destroy(objs->surface);
  if (objs->compositor)
    wl_compositor_destroy(objs->compositor);
  if (objs->registry)
    wl_registry_destroy(objs->registry);
  if (objs->display)
    wl_display_disconnect(objs->display);
}

void ref_display(struct wl_surface* surface, struct wl_buffer* buffer,
                 int32_t width, int32_t height) {
  wl_surface_attach(surface, buffer, 0, 0);
  wl_surface_damage(surface, 0, 0, width, height);
  wl_surface_commit(surface);
}
