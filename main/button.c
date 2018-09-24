#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"

#include "button.h"

static void IRAM_ATTR button_driver_gpio_isr(void* priv) {
	struct button_gpio* button = priv;
	TickType_t now = xTaskGetTickCount();
	if(now < button->debounce_timer || now > button->debounce_timer + BUTTON_DEBOUNCE_INTERVAL) {
		userio_dispatch_event_isr(button->userio, button->action);
	}
	button->debounce_timer = now;
}

esp_err_t button_gpio_alloc(struct button_gpio** retval, struct userio* userio, gpio_num_t gpio, userio_action action) {
	esp_err_t err;
	struct button_gpio* button = calloc(1, sizeof(struct button_gpio));
	if(!button) {
		err = ESP_ERR_NO_MEM;
		goto fail;
	}

	button->userio = userio;
	button->action = action;

	gpio_config_t gpio_conf;
	
	gpio_conf.intr_type = GPIO_PIN_INTR_NEGEDGE;
	gpio_conf.pin_bit_mask = (1ULL<<gpio);
	gpio_conf.mode = GPIO_MODE_INPUT;
	gpio_conf.pull_up_en = 1;
	gpio_conf.pull_down_en = 0;
	gpio_config(&gpio_conf);

	gpio_install_isr_service(0);
	gpio_isr_handler_add(gpio, button_driver_gpio_isr, button);

	*retval = button;
	return ESP_OK;
fail:
	return err;
};
