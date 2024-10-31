/**
 * File:   devices.c
 * Author: AWTK Develop Team
 * Brief:  devices
 *
 * Copyright (c) 2018 - 2023  Guangzhou ZHIYUAN Electronics Co.,Ltd.
 *
 * this program is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose.  see the
 * license file for more details.
 *
 */

/**
 * history:
 * ================================================================
 * 2023-02-27 Shen ZhaoKun <shenzhaokun@zlg.cn> created
 *
 */

#include "devices.h"
#include "tkc/mem.h"
#include "tkc/path.h"
#include "tkc/utils.h"
#include "conf_io/conf_json.h"

#define DEVICES_CONFIG_FILEPATH "config/devices.json"

static uint32_t s_devices_nr = 0;
static device_info_t* s_devices = NULL;
static bool_t s_ext_set = FALSE;

ret_t devices_load(void) {
  char abs_path[MAX_PATH + 1] = {0}, abs_path_with_schema[MAX_PATH + 8] = {0};
  tk_object_t* conf = NULL;
  uint32_t i = 0;
  return_value_if_fail(RET_OK == devices_unload(), RET_FAIL);

  path_prepend_app_root(abs_path, DEVICES_CONFIG_FILEPATH);
  tk_snprintf(abs_path_with_schema, ARRAY_SIZE(abs_path_with_schema) - 1, STR_SCHEMA_FILE "%s",
              abs_path);

  log_debug("%s : path = %s\r\n", __FUNCTION__, abs_path_with_schema);

  conf = conf_json_load(abs_path_with_schema, FALSE);
  return_value_if_fail(conf != NULL, RET_OOM);

  s_devices_nr = tk_object_get_prop_uint32(conf, CONF_SPECIAL_ATTR_SIZE, 0);
  goto_error_if_fail(s_devices_nr > 0);

  s_devices = TKMEM_ZALLOCN(device_info_t, s_devices_nr);
  goto_error_if_fail(s_devices != NULL);

  for (i = 0; i < s_devices_nr; i++) {
    char key[TK_NAME_LEN + 1] = {0};

    tk_snprintf(key, sizeof(key), "[%d]." CONF_SPECIAL_ATTR_NAME, i);
    tk_strncpy(s_devices[i].path, tk_object_get_prop_str(conf, key), sizeof(s_devices[i].path) - 1);

    tk_snprintf(key, sizeof(key), "[%d].type", i);
    tk_strncpy(s_devices[i].type, tk_object_get_prop_str(conf, key), sizeof(s_devices[i].type) - 1);

    log_debug("devices[%d]: path = %s, type = %s\r\n", i, s_devices[i].path, s_devices[i].type);
  }

  TK_OBJECT_UNREF(conf);
  return RET_OK;
error:
  TK_OBJECT_UNREF(conf);
  return RET_FAIL;
}

ret_t devices_unload(void) {
  if (s_devices != NULL) {
    if (!s_ext_set) {
      TKMEM_FREE(s_devices);
    }
    s_devices = NULL;
  }
  s_devices_nr = 0;
  s_ext_set = FALSE;

  return RET_OK;
}

ret_t devices_set(device_info_t* devices, uint32_t nr) {
  uint32_t i = 0;
  return_value_if_fail(devices != NULL && nr > 0, RET_BAD_PARAMS);
  return_value_if_fail(RET_OK == devices_unload(), RET_FAIL);

  s_ext_set = TRUE;
  s_devices_nr = nr;
  s_devices = devices;

  for (i = 0; i < s_devices_nr; i++) {
    log_debug("devices[%d]: path = %s, type = %s\r\n", i, s_devices[i].path, s_devices[i].type);
  }

  return RET_OK;
}

ret_t devices_foreach(device_visit_t visit, void* ctx) {
  uint32_t i = 0;
  return_value_if_fail(visit != NULL, RET_BAD_PARAMS);
  return_value_if_fail(s_devices != NULL, RET_BAD_PARAMS);

  for (i = 0; i < s_devices_nr; i++) {
    if (RET_OK != visit(ctx, &s_devices[i])) {
      break;
    }
  }

  return RET_OK;
}
