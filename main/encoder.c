#include <stdlib.h>

#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "encoder.h"

struct rotate_event {
	struct encoder* enc;
	uint8_t statevec;
};

xQueueHandle encoder_statequeue;
TaskHandle_t encoder_event_task;

static void IRAM_ATTR encoder_gpio_isr(void* priv) {
	struct rotate_event event;
	struct encoder* enc = (struct encoder*)priv;
	enc->statevec = (enc->statevec << 2) & 0xFF;
	if(gpio_get_level(enc->phase_a)) {
		enc->statevec |= 0b01;
	}
	if(gpio_get_level(enc->phase_b)) {
		enc->statevec |= 0b10;
	}
	event.enc = enc;
	event.statevec = enc->statevec;
	xQueueSendFromISR(encoder_statequeue, &event, NULL);
}

void encoder_event_loop(void* arg) {
	struct rotate_event event;
	while(1) {
		if(xQueueReceive(encoder_statequeue, &event, portMAX_DELAY)) {
			uint8_t data = event.statevec;
			printf("Encoder bit pattern: 0b%d%d%d%d%d%d%d%d\n", !!(data & 0b10000000), !!(data & 0b01000000), !!(data & 0b00100000), !!(data & 0b00010000), !!(data & 0b00001000), !!(data & 0b00000100), !!(data & 0b00000010), !!(data & 0b00000001));
		}
	}
}

esp_err_t encoder_start_event_task(void) {
	esp_err_t err;
	encoder_statequeue = xQueueCreate(ENCODER_STATE_QUEUE_LEN, sizeof(struct rotate_event));
	if(!encoder_statequeue) {
		err = ESP_ERR_NO_MEM;
		goto fail_queue;
	}
	if(xTaskCreate(encoder_event_loop, "enc_event_loop", 2048, NULL, 10, &encoder_event_task) != pdPASS) {
		err = ESP_FAIL;
		goto fail_task;
	}

	return ESP_OK;

fail_task:
	vQueueDelete(encoder_statequeue);
fail_queue:
	return err;
}

esp_err_t encoder_alloc(struct encoder** retval, gpio_num_t phase_a, gpio_num_t phase_b) {
	esp_err_t err;
	gpio_config_t gpio_conf;
	struct encoder* enc = calloc(1, sizeof(struct encoder));
	if(!enc) {
		err = ESP_ERR_NO_MEM;
		goto fail;
	}

	enc->phase_a = phase_a;
	enc->phase_b = phase_b;

	gpio_conf.intr_type = GPIO_INTR_ANYEDGE;
	gpio_conf.pin_bit_mask = (1ULL<<phase_a) | (1ULL<<phase_b);
	gpio_conf.mode = GPIO_MODE_INPUT;
	gpio_conf.pull_up_en = 1;
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
