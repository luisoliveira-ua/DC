#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_https_server.h"
#include "driver/i2c_master.h"

// Configurações do Wi-Fi SoftAP
#define EXAMPLE_ESP_WIFI_CHANNEL   6
#define EXAMPLE_MAX_STA_CONN       4

static const char *TAG = "econode_https_app";

static httpd_handle_t server = NULL;

// Símbolos dos certificados embebidos pelo CMake no binário
extern const unsigned char servercert_pem_start[] asm("_binary_servercert_pem_start");
extern const unsigned char servercert_pem_end[]   asm("_binary_servercert_pem_end");
extern const unsigned char prvtkey_pem_start[]    asm("_binary_prvtkey_pem_start");
extern const unsigned char prvtkey_pem_end[]      asm("_binary_prvtkey_pem_end");


// Handler HTTP GET /
static esp_err_t root_get_handler(httpd_req_t *req)
{
    
}

static const httpd_uri_t root = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_get_handler
};

// Arranque do servidor HTTPS
static httpd_handle_t start_webserver(void)
{
    ESP_LOGI(TAG, "A iniciar o servidor HTTPS na porta 443...");
    httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();

    conf.servercert = servercert_pem_start;
    conf.servercert_len = servercert_pem_end - servercert_pem_start;

    conf.prvtkey_pem = prvtkey_pem_start;
    conf.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;

    // Permite ligações de clientes/browsers sem exigir certificado de cliente
    conf.client_cert_authmode_optional = true;

    esp_err_t ret = httpd_ssl_start(&server, &conf);
    if (ESP_OK != ret) {
        ESP_LOGE(TAG, "Erro ao iniciar o servidor HTTPS!");
        return NULL;
    }

    ESP_LOGI(TAG, "A registar handler URI para GET /");
    httpd_register_uri_handler(server, &root);
    return server;
}

// Tratamento de eventos Wi-Fi e IP
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_AP_START) {
            ESP_LOGI(TAG, "SoftAP arrancou. A arrancar o servidor HTTPS...");
            start_webserver();
        } else if (event_id == WIFI_EVENT_AP_STACONNECTED) {
            wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
            ESP_LOGI(TAG, "Estacao ligada: " MACSTR ", AID=%d", MAC2STR(event->mac), event->aid);
        } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
            wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
            ESP_LOGI(TAG, "Estacao desligada: " MACSTR ", AID=%d", MAC2STR(event->mac), event->aid);
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_ASSIGNED_IP_TO_CLIENT) {
            ip_event_ap_staipassigned_t* event = (ip_event_ap_staipassigned_t*) event_data;
            ESP_LOGI(TAG, "Servidor DHCP atribuiu o IP " IPSTR " ao cliente", IP2STR(&event->ip));
        }
    }
}

// Inicialização do modo SoftAP
void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_ASSIGNED_IP_TO_CLIENT, &event_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = true,
            },
        },
    };

    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "SoftAP inicializado. SSID:%s Password:%s Canal:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}

void app_main(void)
{
    // Inicialização do NVS Flash
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Inicialização do sensor DHT20 no barramento I2C
    if (init_dht20() != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao inicializar o sensor DHT20 no barramento I2C");
    } else {
        ESP_LOGI(TAG, "Sensor DHT20 inicializado no barramento I2C com sucesso");
    }

    // Inicialização do Wi-Fi SoftAP
    wifi_init_softap();
}