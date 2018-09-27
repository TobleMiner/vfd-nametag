#ifndef _BUTTON_H_
#define _BUTTON_H_

#include <stdint.h>

#include "freertos/FreeRTOS.h"

#include "driver/gpio.h"

#include "userio.h"

#define BUTTON_DEBOUNCE_INTERVAL (pdMS_TO_TICKS(200))

struct button_gpio {
	struct userio* userio;
	TickType_t debounce_timer;
	userio_action action;
};

esp_err_t button_gpio_alloc(struct button_gpio** retval, struct userio* userio, gpio_num_t gpio, userio_action action);

#endif
