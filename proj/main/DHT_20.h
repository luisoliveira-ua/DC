#ifndef DHT_20
#define DHT_20

#include "driver/i2c_master.h"

void dht20_demo(void);

extern MessageBufferHandle_t buffer_amostras;
typedef struct {
    float temperatura;
    float humidade;
} dht20_data_t;


static void read_dht20(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle);
static void i2c_master_init(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle);


#endif
