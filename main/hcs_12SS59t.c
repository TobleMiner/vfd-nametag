#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

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
	{0x80, 0x87, -40},
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

esp_err_t hcs_set_brightness(struct display* disp, unsigned brightness) {
	return hcs_cmd(DISP_TO_HCS(disp), HCS_12SS59T_DUTY, (uint8_t)brightness);
}

esp_err_t hcs_blank(struct display* disp, bool blank) {
	return hcs_cmd(DISP_TO_HCS(disp), HCS_12SS59T_LIGHT, blank ? HCS_12SS59T_LIGHT_OFF : HCS_12SS59T_LIGHT_NORM);
}

esp_err_t hcs_display_bin(struct display* disp, char* str, size_t len) {
	struct hcs_12SS59t* hcs = DISP_TO_HCS(disp);
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

esp_err_t hcs_display(struct display* disp, char* str) {
	return hcs_display_bin(disp, str, strlen(str));
}

esp_err_t hcs_get_display_width(struct display* disp, size_t* width) {
	(void)disp;
	*width = HCS_12SS59T_NUM_CHARS;
	return ESP_OK;
}

static size_t hcs_get_spinner_char_cnt(struct display* disp) {
	(void)disp;
	return 8;
}

static char hcs_get_spinner_char(struct display* disp, size_t index) {
	(void)disp;
	return (char)(0x80 + index);
}

void hcs_free(struct display* disp) {
	struct hcs_12SS59t* hcs = DISP_TO_HCS(disp);
	spi_bus_remove_device(hcs->spi_dev);
	free(hcs);
}

const struct display_ops hcs_ops = {
	.blank = hcs_blank,
	.set_brightness = hcs_set_brightness,
};

const struct display_ops_text hcs_text_ops = {
	.get_width = hcs_get_display_width,
	.display = hcs_display,
	.display_bin = hcs_display_bin,
	.get_spinner_char_cnt = hcs_get_spinner_char_cnt,
	.get_spinner_char = hcs_get_spinner_char,
};

esp_err_t hcs_alloc(struct display** retval, spi_host_device_t spi_phy, gpio_num_t gpio_cs, gpio_num_t gpio_reset) {
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

	hcs->disp.capabilities = DISPLAY_CAP_TEXT;
	hcs->disp.ops = hcs_ops;
	hcs->disp.text_ops = hcs_text_ops;

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
	if((err = hcs_set_brightness(&hcs->disp, 15))) {
		goto fail_spi;
	}
	if((hcs_blank(&hcs->disp, false))) {
		goto fail_spi;
	}

	*retval = &hcs->disp;
	return ESP_OK;

fail_spi:
	spi_bus_remove_device(hcs->spi_dev);
fail_alloc:
	free(hcs);
fail:
	return err;
}
