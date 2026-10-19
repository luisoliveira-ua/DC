#ifndef SERVIDOR
#define SERVIDOR
s

#include "esp_err.h"
#include "esp_https_server.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "driver/i2c_master.h"

// Inicialização do Wi-Fi em modo SoftAP
void wifi_init_softap(void);

// Inicialização do sensor DHT20 no barramento I2C
esp_err_t init_dht20(void);

// Arranque do servidor HTTPS
// Devolve o identificador do servidor iniciado
httpd_handle_t start_webserver(void);

#endif 