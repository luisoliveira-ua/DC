#include "led.h"

#include "driver/gpio.h"
#include "led_strip.h"

#define GPIO_LED_RGB GPIO_NUM_8


static led_strip_handle_t led = NULL;

/**
 * Inicializa o LED WS2812 integrado usando o periférico RMT.
 */
esp_err_t led_init(void)
{
    const led_strip_config_t strip_config = {
        .strip_gpio_num = GPIO_LED_RGB,
        .max_leds = 1,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags.invert_out = false,
    };

    const led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .mem_block_symbols = 64,
        .flags.with_dma = false,
    };

    return led_strip_new_rmt_device(&strip_config, &rmt_config, &led);
}

/** Define a cor RGB do único LED integrado e atualiza a saída. */
esp_err_t led_cor(uint8_t vermelho, uint8_t verde, uint8_t azul)
{
    if (led == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = led_strip_set_pixel(led, 0, vermelho, verde, azul);
    if (err != ESP_OK) {
        return err;
    }

    return led_strip_refresh(led);
}

/** Apaga o LED integrado. */
esp_err_t led_apagar(void)
{
    return led_cor(0, 0, 0);
}
