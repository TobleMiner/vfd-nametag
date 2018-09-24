#include <stdlib.h>

#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "encoder.h"

int8_t encoder_dt_lookup[16] = {
	0 /* 0b0000 */,
	0 /* 0b0001 */,	
	0 /* 0b0010 */,	
	0 /* 0b0011 */,	
	1 /* 0b0100 */,	
	0 /* 0b0101 */,	
	0 /* 0b0110 */,	
	-1 /* 0b0111 */,	
	1 /* 0b1000 */,	
	0 /* 0b1001 */,	
	0 /* 0b1010 */,	
	-1 /* 0b1011 */,	
	0 /* 0b1100 */,	
	0 /* 0b1101 */,	
	0 /* 0b1110 */,	
	0 /* 0b1111 */,	
};

static void IRAM_ATTR encoder_gpio_isr(void* priv) {
	int8_t dt_turnon, dt_turnoff;
	struct encoder* enc = (struct encoder*)priv;

	enc->statevec = (enc->statevec << 2) & 0xFF;
	if(gpio_get_level(enc->phase_a)) {
		enc->statevec |= 0b01;
	}
	if(gpio_get_level(enc->phase_b)) {
		enc->statevec |= 0b10;
	}

	dt_turnon = encoder_dt_lookup[(enc->statevec >> 4) & 0x0F];
	dt_turnoff = encoder_dt_lookup[enc->statevec & 0x0F];

	if(dt_turnon == 0 && dt_turnoff == -1) {
		userio_dispatch_event_isr(enc->userio, USERIO_ACTION_NEXT);
	}
	else if(dt_turnon == 1 && dt_turnoff == 0) {
		userio_dispatch_event_isr(enc->userio, USERIO_ACTION_PREV);
	}
}

esp_err_t encoder_alloc(struct encoder** retval, struct userio* userio, gpio_num_t phase_a, gpio_num_t phase_b) {
	esp_err_t err;
	gpio_config_t gpio_conf;
	struct encoder* enc = calloc(1, sizeof(struct encoder));
	if(!enc) {
		err = ESP_ERR_NO_MEM;
		goto fail;
	}

	enc->phase_a = phase_a;
	enc->phase_b = phase_b;
	enc->userio = userio;

	gpio_conf.intr_type = GPIO_INTR_ANYEDGE;
	gpio_conf.pin_bit_mask = (1ULL<<phase_a) | (1ULL<<phase_b);
	gpio_conf.mode = GPIO_MODE_INPUT;
	gpio_conf.pull_up_en = 1;
	gpio_conf.pull_down_en = 0;
	gpio_config(&gpio_conf);

	gpio_install_isr_service(0);
	gpio_isr_handler_add(phase_a, encoder_gpio_isr, enc);
	gpio_isr_handler_add(phase_b, encoder_gpio_isr, enc);

	*retval = enc;
	return ESP_OK;

//fail_alloc:
//	free(enc);
fail:
	return err;
}
