#include <stdlib.h>

#include "esp_err.h"

#include "modal_wait.h"
#include "modal_wait_render.h"
#include "ui.h"

extern struct ui_element_render modal_wait_render_text;

struct ui_element_def modal_wait_def = {
	.renders = {
		&modal_wait_render_text,
		NULL
	},
};

esp_err_t modal_wait_alloc(struct modal_wait** retval, const char* text, struct ui_element* prev_elem) {
	esp_err_t err;
	struct modal_wait* modal = calloc(1, sizeof(struct modal_wait));
	if(!modal) {
		err = ESP_ERR_NO_MEM;
		goto fail;
	}

	ui_element_init(&modal->elem, &modal_wait_def);

	modal->prev_elem = prev_elem;
	modal->text = text;

	*retval = modal;
	return ESP_OK;

fail:
	return err;
}

void modal_wait_free(struct modal_wait* modal, struct ui* ui) {
	if(ui && modal->prev_elem) {
		ui_set_active_element(ui, modal->prev_elem);
	}
	free(modal);
}
