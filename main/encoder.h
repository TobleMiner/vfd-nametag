#ifndef _ENCODER_H_
#define _ENCODER_H_

#include "driver/gpio.h"

#include "userio.h"

struct encoder {
	uint8_t statevec;

	gpio_num_t phase_a;
	gpio_num_t phase_b;

	struct userio* userio;
};

esp_err_t encoder_alloc(struct encoder** retval, struct userio* userio, gpio_num_t phase_a, gpio_num_t phase_b);

#endif
