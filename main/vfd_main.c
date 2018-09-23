#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "hcs_12SS59t.h"

#define PIN_NUM_MISO 25
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  19
#define PIN_NUM_CS   22

#define PIN_NUM_RST  18

void app_main()
{
	esp_err_t err;

	printf("Starting VFD name badge app\n");

	printf("Initializing SPI...\n");
	spi_bus_config_t buscfg = {
		.miso_io_num = PIN_NUM_MISO,
		.mosi_io_num = PIN_NUM_MOSI,
		.sclk_io_num = PIN_NUM_CLK,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
		.max_transfer_sz = 32
	};

	err = spi_bus_initialize(HSPI_HOST, &buscfg, 0);
	printf("SPI init finish, %d\n", err);
	ESP_ERROR_CHECK(err);
	printf("SPI initialized\n");

	struct display* disp;

	printf("Initializing VFD...\n");
	err = hcs_alloc(&disp, HSPI_HOST, PIN_NUM_CS, PIN_NUM_RST);
	printf("VFD init finish, %d\n", err);
	ESP_ERROR_CHECK(err);
	printf("VFD initialized\n");

	err = display_text_display(disp, "Hello_World");
	ESP_ERROR_CHECK(err);

	uint8_t brightness = 1;
	uint8_t direction = 0;

	while(1) {
		vTaskDelay(60 / portTICK_PERIOD_MS);
		if(direction) {
			if(brightness-- <= 1) {
				direction = 0;
			}
		} else {
			if(brightness++ >= 14) {
				direction = 1;
			}
		}
		display_set_brightness(disp, brightness);
	}
}
