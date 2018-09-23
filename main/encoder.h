#ifndef _ENCODER_H_
#define _ENCODER_H_

#include "driver/gpio.h"

#define ENCODER_DIR_CCKW -1
#define ENCODER_DIR_CLKW 1

#define ENCODER_STATE_QUEUE_LEN 32

struct encoder {
	uint8_t statevec;

	gpio_num_t phase_a;
	gpio_num_t phase_b;

	void* cb_priv;
	void (*callback)(int8_t dir, void* priv);
};

esp_err_t encoder_start_event_task(void);
esp_err_t encoder_alloc(struct encoder** retval, gpio_num_t phase_a, gpio_num_t phase_b);

#endif
