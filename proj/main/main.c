#include "cartao.h"
#include "DHT_20.h"

MessageBufferHandle_t buffer_amostras = NULL;

void app_main(void)
{
    // Espaço para 8 amostras e o cabeçalho de tamanho de cada mensagem.
    buffer_amostras = xMessageBufferCreate(8 * (sizeof(dht20_data_t) + sizeof(size_t)));
    configASSERT(buffer_amostras != NULL);

    ESP_ERROR_CHECK(cartao_init());
}
