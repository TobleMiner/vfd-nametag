#ifndef _HCS_12SS59T_
#define _HCS_12SS59T_

#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#define HCS_12SS59T_DCRAM_WR	0x10
#define HCS_12SS59T_CGRAM_WR	0x20
#define HCS_12SS59T_ADRAM_WR	0x30
#define HCS_12SS59T_DUTY	0x50
#define HCS_12SS59T_NUMDIGIT	0x60
#define HCS_12SS59T_LIGHT	0x70

#define HCS_12SS59T_LIGHT_NORM	0x00
#define HCS_12SS59T_LIGHT_OFF	0x01
#define HCS_12SS59T_LIGHT_ON	0x02


#define HCS_12SS59T_NUM_CHARS	12

struct hcs_12SS59t {
	gpio_num_t gpio_reset;
	spi_device_handle_t spi_dev;
};

esp_err_t hcs_alloc(struct hcs_12SS59t** retval, spi_host_device_t spi_phy, gpio_num_t gpio_cs, gpio_num_t gpio_reset);
void hcs_free(struct hcs_12SS59t* hcs);

esp_err_t hcs_display(struct hcs_12SS59t* hcs, char* str, size_t len);
esp_err_t hcs_set_brightness(struct hcs_12SS59t* hcs, uint8_t brightness);


#endif
