#ifndef _UI_H_
#define _UI_H_

#include <sys/types.h>
#include <stdbool.h>

#include "esp_err.h"

#include "display.h"
#include "list.h"
#include "userio.h"


typedef uint8_t ui_render_flag;

struct ui {
	struct display* disp;

	struct list_head ui_elements;

	struct ui_element* active_element;

	TickType_t last_animate_tick;
};

struct ui_element_ops {
	esp_err_t (*action_performed)(struct ui* ui, struct ui_element* elem, userio_action action);
};

struct ui_element_render;

struct ui_element_render_ops {
	esp_err_t (*render)(struct ui_element_render* render, struct ui_element* elem, struct display* disp, TickType_t last_tick, TickType_t ticks);
};

struct ui_element_def;

struct ui_element {
	struct list_head list;

	struct ui_element_def* def;

	bool needs_render;
};

struct ui_element_render {
	struct {
		ui_render_flag text:1;
		ui_render_flag graphics:1;
	} flags;

	struct ui_element_render_ops* ops;
};

struct ui_element_def {
	struct ui_element_ops* ops;

	struct ui_element_render* renders[];
};

#define ui_add_element(element, ui) \
	LIST_APPEND(&(element)->list, &(ui)->ui_elements)

#define ui_remove_element(element) \
	LIST_DELETE(&(element)->list)

inline esp_err_t ui_action_performed(struct ui* ui, userio_action action) {
	struct ui_element* elem = ui->active_element;
	if(elem->def->ops && elem->def->ops->action_performed) {
		return elem->def->ops->action_performed(ui, elem, action);
	}
	return ESP_OK;
}

#define ui_element_update(elem) \
	(elem)->needs_render = true

#define ui_set_active_element(ui, elem) \
	(ui)->active_element = elem;

#define ui_get_active_element(ui) \
	((ui)->active_element)

esp_err_t ui_alloc(struct ui** retval, struct display* disp);
void ui_element_init(struct ui_element* elem, struct ui_element_def* def);
void ui_element_render_init(struct ui_element_render* render, struct ui_element_render_ops* ops);
esp_err_t ui_do_render(struct ui* ui, TickType_t ticks);

#endif
