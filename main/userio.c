#include <stdlib.h>

#include "esp_err.h"

#include "userio.h"

esp_err_t userio_alloc(struct userio** retval) {
	esp_err_t err;
	struct userio* userio = calloc(1, sizeof(struct userio));
	if(!userio) {
		err = ESP_ERR_NO_MEM;
		goto fail;
	}

	userio->eventqueue = xQueueCreate(USERIO_EVENT_QUEUE_LEN, sizeof(struct userio_event));
	if(!userio->eventqueue) {
		err = ESP_ERR_NO_MEM;
		goto fail_alloc;
	}

	*retval = userio;
	return ESP_OK;

fail_alloc:
	free(userio);
fail:
	return err;
}

BaseType_t userio_wait_event(struct userio* userio, userio_action* action) {
	struct userio_event event;
	BaseType_t ret = xQueueReceive(userio->eventqueue, &event, portMAX_DELAY);
	if(ret) {
		*action = event.action;
	}
	return ret;
}
