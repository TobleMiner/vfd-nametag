#ifndef _PROGRESS_H_
#define _PROGRESS_H_

#include "ui.h"
#include "util.h"

struct modal_wait {
	struct ui_element elem;
	struct ui_element* prev_elem;

	const char* text;
};

#define UI_ELEMENT_TO_MODAL_WAIT(elem) \
	container_of((elem), struct modal_wait, elem)

#define modal_wait_show(modal, ui) \
	ui_set_active_element(ui, &(modal)->elem)

esp_err_t modal_wait_alloc(struct modal_wait** retval, const char* text, struct ui_element* prev_elem);
void modal_wait_free(struct modal_wait* prog, struct ui* ui);

#endif
