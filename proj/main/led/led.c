#include "led.h"


static uint8_t led_state;

#ifdef CONFIG_BLINK_LED_STRIP
static led_strip_handle_t led_strip;
#endif


uint8_t get_led_state(void) { return led_state; }

#ifdef CONFIG_BLINK_LED_STRIP

void led_set_color(uint8_t red, uint8_t green, uint8_t blue)
{
    led_strip_set_pixel(led_strip, 0, red, green, blue);
    led_strip_refresh(led_strip);
    led_state = (red || green || blue);
}

void led_on(void)
{
    
    led_set_color(16, 16, 16);
}

void led_off(void)
{
    led_strip_clear(led_strip);
    led_state = false;
}

void led_init(void)
{
    ESP_LOGI(TAG, "example configured to blink addressable led!");

    led_strip_config_t strip_config = {
        .strip_gpio_num = CONFIG_BLINK_GPIO,
        .max_leds = 1,
    };

#if CONFIG_BLINK_LED_STRIP_BACKEND_RMT
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(
        led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
#elif CONFIG_BLINK_LED_STRIP_BACKEND_SPI
    led_strip_spi_config_t spi_config = {
        .spi_bus = SPI2_HOST,
        .flags.with_dma = true,
    };
    ESP_ERROR_CHECK(
        led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
#else
#error "unsupported LED strip backend"
#endif

    led_off();
}

#elif CONFIG_BLINK_LED_GPIO

/* GPIO simples não suporta cores: esta função apenas liga o LED. */
void led_set_color(uint8_t red, uint8_t green, uint8_t blue)
{
    bool on = (red || green || blue);
    gpio_set_level(CONFIG_BLINK_GPIO, on);
    led_state = on;
}

void led_on(void)
{
    led_set_color(16, 16, 16);
}

void led_off(void)
{
    gpio_set_level(CONFIG_BLINK_GPIO, false);
    led_state = false;
}

void led_init(void)
{
    ESP_LOGI(TAG, "example configured to blink gpio led!");
    gpio_reset_pin(CONFIG_BLINK_GPIO);
    gpio_set_direction(CONFIG_BLINK_GPIO, GPIO_MODE_OUTPUT);
    led_off();
}

#else
#error "unsupported LED type"
#endif