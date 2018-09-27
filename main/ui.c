#include <stdlib.h>

#include "ui.h"

esp_err_t ui_alloc(struct ui** retval, struct display* disp) {
	esp_err_t err;
	struct ui* ui = calloc(1, sizeof(struct ui));
	if(!ui) {
		err = ESP_ERR_NO_MEM;
		goto fail;
	}

	ui->disp = disp;
	INIT_LIST_HEAD(ui->ui_elements);

	*retval = ui;
	return ESP_OK;

fail:
	return err;
}

void ui_element_init(struct ui_element* elem, struct ui_element_ops* ops) {
	INIT_LIST_HEAD(elem->renders);
	elem->ops = ops;
}

void ui_element_render_init(struct ui_element_render* render, struct ui_element_render_ops* ops) {
	INIT_LIST_HEAD(render->list);
	render->ops = ops;
}

static struct ui_element_render* ui_element_find_render(struct ui_element* elem, struct display* disp) {
	struct list_head* cursor;

	LIST_FOR_EACH(cursor, &elem->renders) {
		struct ui_element_render* render = LIST_GET_ENTRY(cursor, struct ui_element_render, list);
		if((disp->capabilities & DISPLAY_CAP_TEXT) && render->flags.text) {
			return render;
		}
		if((disp->capabilities & DISPLAY_CAP_GRAPHICS) && render->flags.graphics) {
			return render;
		}
	}

	return NULL;
}

static esp_err_t ui_element_do_render(struct ui_element* elem, struct display* disp) {
	struct ui_element_render* render = ui_element_find_render(elem, disp);
	if(!render) {
		return ESP_ERR_NOT_SUPPORTED;
	}

	return render->ops->render(render, elem, disp);
}

esp_err_t ui_do_render(struct ui* ui) {
	if(!ui->active_element) {
		return ESP_ERR_INVALID_STATE;
	}

/* TODO: Honor render required flag
	if(!ui->active_element->needs_render) {
		return ESP_OK;
	}
*/
	return ui_element_do_render(ui->active_element, ui->disp);
}
