#ifndef _USERIO_H_
#define _USERIO_H_

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define USERIO_ACTION_NEXT	0b00000001
#define USERIO_ACTION_PREV	0b00000010
#define USERIO_ACTION_SELECT	0b00000100
#define USERIO_ACTION_BACK	0b00001000

#define USERIO_EVENT_QUEUE_LEN 32

typedef uint8_t userio_action;

struct userio {
	xQueueHandle eventqueue;	
};

struct userio_event {
	userio_action action;
}; 

esp_err_t userio_alloc(struct userio** retval);
BaseType_t userio_wait_event(struct userio* userio, userio_action* action);

inline void userio_dispatch_event_isr(struct userio* userio, userio_action action) {
	struct userio_event event;
	event.action = action;
	xQueueSendFromISR(userio->eventqueue, &event, NULL);
}

#endif
