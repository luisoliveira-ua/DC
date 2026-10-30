#include "app_config.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "esp_log.h"

#define CONFIG_FILE_PATH       "/sdcard/config.json"
#define CONFIG_FILE_MAX_SIZE   1024
#define TELEMETRY_BASE_SIZE    64
#define UUID_STRING_SIZE       37

static const char *TAG = "app_config";

typedef struct {
    uint32_t periodo_leitura_s;
    uint32_t node_id;
    char telemetry_base[TELEMETRY_BASE_SIZE];
    char gatt_service_uuid[UUID_STRING_SIZE];
} app_config_data_t;

static app_config_data_t s_config;
static bool s_config_loaded;

static bool read_uint(const cJSON *object, const char *key,
                      uint32_t minimum, uint32_t maximum, uint32_t *value)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(object, key);
    if (!cJSON_IsNumber(item) || item->valuedouble < minimum ||
        item->valuedouble > maximum ||
        (double)(uint32_t)item->valuedouble != item->valuedouble) {
        return false;
    }

    *value = (uint32_t)item->valuedouble;
    return true;
}

static bool read_string(const cJSON *object, const char *key,
                        char *destination, size_t destination_size)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(object, key);
    if (!cJSON_IsString(item) || item->valuestring == NULL) {
        return false;
    }

    size_t length = strlen(item->valuestring);
    if (length == 0 || length >= destination_size) {
        return false;
    }

    memcpy(destination, item->valuestring, length + 1);
    return true;
}

static esp_err_t read_config_file(char **contents)
{
    FILE *file = fopen(CONFIG_FILE_PATH, "r");
    if (file == NULL) {
        ESP_LOGE(TAG, "Não foi possível abrir %s", CONFIG_FILE_PATH);
        return ESP_ERR_NOT_FOUND;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return ESP_FAIL;
    }

    long file_size = ftell(file);
    if (file_size <= 0 || file_size > CONFIG_FILE_MAX_SIZE ||
        fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        ESP_LOGE(TAG, "Tamanho inválido para %s", CONFIG_FILE_PATH);
        return ESP_ERR_INVALID_SIZE;
    }

    char *buffer = calloc((size_t)file_size + 1, 1);
    if (buffer == NULL) {
        fclose(file);
        return ESP_ERR_NO_MEM;
    }

    size_t bytes_read = fread(buffer, 1, (size_t)file_size, file);
    fclose(file);
    if (bytes_read != (size_t)file_size) {
        free(buffer);
        ESP_LOGE(TAG, "Erro ao ler %s", CONFIG_FILE_PATH);
        return ESP_FAIL;
    }

 
    if (bytes_read >= 3 && (unsigned char)buffer[0] == 0xEF &&
        (unsigned char)buffer[1] == 0xBB &&
        (unsigned char)buffer[2] == 0xBF) {
        memmove(buffer, buffer + 3, bytes_read - 3);
        buffer[bytes_read - 3] = '\0';
    }

    *contents = buffer;
    return ESP_OK;
}

esp_err_t app_config_init(void)
{
    char *contents = NULL;
    esp_err_t result = read_config_file(&contents);
    if (result != ESP_OK) {
        return result;
    }

    cJSON *root = cJSON_Parse(contents);
    free(contents);
    if (!cJSON_IsObject(root)) {
        cJSON_Delete(root);
        ESP_LOGE(TAG, "O ficheiro de configuração não contém um objeto JSON válido");
        return ESP_ERR_INVALID_RESPONSE;
    }

    uint32_t config_version = 0;
    app_config_data_t parsed = {0};
    bool valid = read_uint(root, "config_version", 1, 1, &config_version) &&
                 config_version == 1 &&
                 read_uint(root, "periodo_leitura_s", 1, 3600,
                           &parsed.periodo_leitura_s) &&
                 read_uint(root, "node_id", 1000, 0xFFFF, &parsed.node_id) &&
                 read_string(root, "telemetry_base", parsed.telemetry_base,
                             sizeof(parsed.telemetry_base)) &&
                 read_string(root, "gatt_config_service_uuid",
                             parsed.gatt_service_uuid,
                             sizeof(parsed.gatt_service_uuid));
    cJSON_Delete(root);

    if (valid) {
        char expected_base[TELEMETRY_BASE_SIZE];
        char expected_uuid[UUID_STRING_SIZE];
        snprintf(expected_base, sizeof(expected_base), "econode/%lu/",
                 (unsigned long)parsed.node_id);
        snprintf(expected_uuid, sizeof(expected_uuid),
                 "E0C0DE00-0000-4000-8000-00000000%04lX",
                 (unsigned long)parsed.node_id);
        valid = strcmp(parsed.telemetry_base, expected_base) == 0 &&
                strcmp(parsed.gatt_service_uuid, expected_uuid) == 0;
    }

    if (!valid) {
        ESP_LOGE(TAG, "Configuração inválida; verifica versão, campos e limites");
        return ESP_ERR_INVALID_ARG;
    }

    s_config = parsed;
    s_config_loaded = true;
    ESP_LOGI(TAG, "Configuração carregada (período: %lu s, node_id: %lu)",
             (unsigned long)s_config.periodo_leitura_s,
             (unsigned long)s_config.node_id);
    return ESP_OK;
}

uint32_t app_config_get_periodo_s(void)
{
    return s_config_loaded ? s_config.periodo_leitura_s : 0;
}

uint32_t app_config_get_node_id(void)
{
    return s_config_loaded ? s_config.node_id : 0;
}

const char *app_config_get_telemetry_base(void)
{
    return s_config_loaded ? s_config.telemetry_base : NULL;
}

const char *app_config_get_gatt_service_uuid(void)
{
    return s_config_loaded ? s_config.gatt_service_uuid : NULL;
}
