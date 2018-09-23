#ifndef _HCS_12SS59T_
#define _HCS_12SS59T_

#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "display.h"
#include "util.h"

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

	struct display disp;
};

#define DISP_TO_HCS(_disp) (container_of((_disp), struct hcs_12SS59t, disp))


esp_err_t hcs_alloc(struct display** retval, spi_host_device_t spi_phy, gpio_num_t gpio_cs, gpio_num_t gpio_reset);
void hcs_free(struct display* disp);

#endif
