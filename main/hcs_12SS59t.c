#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "hcs_12SS59t.h"
#include "units.h"


static void hcs_reset(struct hcs_12SS59t* hcs) {
	gpio_set_level(hcs->gpio_reset, 0);
	vTaskDelay(1); // We would need only around 250ns of delay here
	gpio_set_level(hcs->gpio_reset, 1);
}

static esp_err_t hcs_cmd_data(struct hcs_12SS59t* hcs, uint8_t cmd, uint8_t addr, uint8_t* data, size_t len) {
	spi_transaction_ext_t trans;

	memset(&trans, 0, sizeof(trans));
	trans.base.flags = SPI_TRANS_VARIABLE_CMD;
	trans.command_bits = 8;
	trans.base.cmd = cmd | addr;
	trans.base.tx_buffer = data;
	trans.base.length = len * 8;
	return spi_device_transmit(hcs->spi_dev, &trans.base);
}

static esp_err_t hcs_cmd(struct hcs_12SS59t* hcs, uint8_t cmd, uint8_t arg) {
	spi_transaction_t trans;
	uint8_t data = cmd | arg;

	memset(&trans, 0, sizeof(trans));
	trans.tx_buffer = &data;
	trans.length = 8;
	return spi_device_transmit(hcs->spi_dev, &trans);
}

struct hcs_xlate_entry {
	uint8_t from;
	uint8_t to;
	int8_t offset;
};

struct hcs_xlate_entry hcs_xlate_table[] = {
	{0, 16, 0},
	{'<', '<', -4},
	{'>', '>', -5},
	{'@', '_', -48},
	{' ', '?', 16},
	{'a', 'z', -80},
	{0, 0, 0}
};

static uint8_t hcs_xlate_char(char c) {
	uint8_t val = (uint8_t)c;
	struct hcs_xlate_entry* xlate_entry = hcs_xlate_table;
	while(xlate_entry->from || xlate_entry->to || xlate_entry->offset) {
		if(val >= xlate_entry->from && val <= xlate_entry->to) {
			return val + xlate_entry->offset;
		}
		xlate_entry++;
	}
	return 0xFF;
}

static void hcs_xlate_str(uint8_t* dst, char* src, size_t len) {
	while(len-- > 0) {
		*dst++ = hcs_xlate_char(*src++);
	}
}

esp_err_t hcs_set_brightness(struct hcs_12SS59t* hcs, uint8_t brightness) {
	return hcs_cmd(hcs, HCS_12SS59T_DUTY, brightness);
}

esp_err_t hcs_alloc(struct hcs_12SS59t** retval, spi_host_device_t spi_phy, gpio_num_t gpio_cs, gpio_num_t gpio_reset) {
	esp_err_t err;
	gpio_config_t gpio_conf;
	spi_device_interface_config_t spi_conf;

	struct hcs_12SS59t* hcs = calloc(1, sizeof(struct hcs_12SS59t));
	if(!hcs) {
		err = ESP_ERR_NO_MEM;
		goto fail;
	}

	memset(hcs, 0, sizeof(*hcs));
	memset(&spi_conf, 0, sizeof(spi_conf));
	memset(&gpio_conf, 0, sizeof(gpio_config_t));

	gpio_conf.intr_type = GPIO_PIN_INTR_DISABLE;
	gpio_conf.mode = GPIO_MODE_OUTPUT;
	gpio_conf.pin_bit_mask = (1ULL<<gpio_reset);
	gpio_conf.pull_down_en = 0;
	gpio_conf.pull_up_en = 0;
	if((err = gpio_config(&gpio_conf))) {
		goto fail_alloc;
	}
	hcs->gpio_reset = gpio_reset;

	spi_conf.clock_speed_hz		= KHZ_TO_HZ(100);
	spi_conf.mode			= 3; // CPOL = 1, CPHA = 1
	spi_conf.spics_io_num		= gpio_cs;
	spi_conf.queue_size		= 8;
	spi_conf.cs_ena_pretrans	= 4;
	spi_conf.cs_ena_posttrans	= 4;
	spi_conf.flags			= SPI_DEVICE_TXBIT_LSBFIRST | SPI_DEVICE_HALFDUPLEX;

	if((err = spi_bus_add_device(spi_phy, &spi_conf, &hcs->spi_dev))) {
		goto fail_alloc;
	}

	hcs_reset(hcs);

	if((err = hcs_cmd(hcs, HCS_12SS59T_NUMDIGIT, HCS_12SS59T_NUM_CHARS))) {
		goto fail_spi;
	}
	if((err = hcs_set_brightness(hcs, 15))) {
		goto fail_spi;
	}
	if((hcs_cmd(hcs, HCS_12SS59T_LIGHT, HCS_12SS59T_LIGHT_NORM))) {
		goto fail_spi;
	}

	*retval = hcs;
	return ESP_OK;

fail_spi:
	spi_bus_remove_device(hcs->spi_dev);
fail_alloc:
	free(hcs);
fail:
	return err;
}

void hcs_free(struct hcs_12SS59t* hcs) {
	spi_bus_remove_device(hcs->spi_dev);
	free(hcs);
}

#ifndef min
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif

#ifndef swp
#define swp(_a, _b) \
	do { \
		typeof((_a)) tmp = (_a); \
		(_a) = (_b); \
		(_b) = tmp; \
	} while(0)
#endif

#define ARRAY_REVERSE(_arr, _len) \
	do { \
		typeof((_len)) i; \
		for(i = 0; i < _len / 2; i++) swp(((_arr)[i]), ((_arr)[_len - i - 1])); \
	} while(0)

						

esp_err_t hcs_display(struct hcs_12SS59t* hcs, char* str, size_t len) {
	uint8_t data[HCS_12SS59T_NUM_CHARS];
	uint8_t* data_ptr = data;
	ssize_t diff = HCS_12SS59T_NUM_CHARS - len;

	memset(data, hcs_xlate_char(' '), HCS_12SS59T_NUM_CHARS);
	len = min(len, HCS_12SS59T_NUM_CHARS);
	hcs_xlate_str(data_ptr, str, len);

	if(diff > 0) {
		data_ptr += diff;
	}

	ARRAY_REVERSE(data, sizeof(data));
	return hcs_cmd_data(hcs, HCS_12SS59T_DCRAM_WR, 0, data, sizeof(data));
}
