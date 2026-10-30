#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>
#include "esp_err.h"

/* Lê e valida a configuração em /sdcard/config.json. */
esp_err_t app_config_init(void);

/* Os getters consultam a configuração central, sem cópias por módulo. */
uint32_t app_config_get_periodo_s(void);
uint32_t app_config_get_node_id(void);
const char *app_config_get_telemetry_base(void);
const char *app_config_get_gatt_service_uuid(void);

#endif
