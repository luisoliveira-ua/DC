#ifndef DHT_20
#define DHT_20

#include <stdint.h>
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/message_buffer.h"

void dht20(void *arg);

extern MessageBufferHandle_t buffer_amostras;
typedef struct {
    float temperatura;
    float humidade;
    int64_t time;
} dht20_data_t;


#endif
