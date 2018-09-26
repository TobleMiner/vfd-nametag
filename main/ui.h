#ifndef _UI_H_
#define _UI_H_

#include <sys/types.h>
#include <stdbool.h>

#include "esp_err.h"

#include "display.h"
#include "list.h"


typedef uint8_t ui_render_flag;

struct ui {
	struct display* disp;

	struct list_head ui_elements;

	struct ui_element* active_element;
};

struct ui_element {
	struct list_head list;

	struct list_head renders;

	bool needs_render;
};

struct ui_element_render;

struct ui_element_render_ops {
	esp_err_t (*render)(struct ui_element_render* render, struct ui_element* elem, struct display* disp);
};

struct ui_element_render {
	struct list_head list;

	struct {
		ui_render_flag text:1;
		ui_render_flag graphics:1;
	} flags;

	struct ui_element_render_ops* ops;
};

#define ui_add_element(element, ui) \
	LIST_APPEND(&(element)->list, &(ui)->ui_elements)

#define ui_element_attach_render(render, elem) \
	LIST_APPEND(&(render)->list, &(elem)->renders)

#define ui_element_update(elem) \
	(elem)->needs_render = true

#define ui_set_active_element(ui, elem) \
	(ui)->active_element = elem;


esp_err_t ui_alloc(struct ui** retval, struct display* disp);
void ui_element_init(struct ui_element* elem);
void ui_element_render_init(struct ui_element_render* render, struct ui_element_render_ops* ops);
esp_err_t ui_do_render(struct ui* ui);

#endif
