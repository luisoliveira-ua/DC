#include "cartao.h"
#include "DHT_20.h"
#include "freertos/task.h"

MessageBufferHandle_t buffer_amostras = NULL;

void app_main(void)
{
    buffer_amostras = xMessageBufferCreate(8 * (sizeof(dht20_data_t) + sizeof(size_t)));
    configASSERT(buffer_amostras != NULL);

    ESP_ERROR_CHECK(cartao_init());
    BaseType_t resultado = xTaskCreate(dht20, "dht20", 4096, NULL, 5, NULL);
    configASSERT(resultado == pdPASS);
}
