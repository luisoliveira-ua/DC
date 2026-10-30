#include "cartao.h"
#include "DHT_20.h"
<<<<<<< Updated upstream
#include "config/app_config.h"
#include "esp_log.h"
#include "freertos/task.h"

static const char *TAG = "main";
=======
#include "freertos/task.h"
>>>>>>> Stashed changes

MessageBufferHandle_t buffer_amostras = NULL;

static void dht20_task(void *arg)
{
    (void)arg;
    dht20();
    vTaskDelete(NULL);
}

void app_main(void)
{
    // Espaço para 8 amostras e o cabeçalho de tamanho de cada mensagem.
    buffer_amostras = xMessageBufferCreate(8 * (sizeof(dht20_data_t) + sizeof(size_t)));
    configASSERT(buffer_amostras != NULL);

    ESP_ERROR_CHECK(cartao_init());
<<<<<<< Updated upstream

    esp_err_t err = app_config_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Arranque interrompido: configuração do microSD inválida");
        return;
    }

    if (xTaskCreate(dht20_task, "dht20", 4096, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Não foi possível criar a tarefa do DHT20");
    }
=======
    BaseType_t resultado = xTaskCreate(dht20, "dht20", 4096, NULL, 5, NULL);
    configASSERT(resultado == pdPASS);
>>>>>>> Stashed changes
}
