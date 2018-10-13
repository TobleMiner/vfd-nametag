#include <stdlib.h>

#include "tag_app_render.h"
#include "tag_app.h"
#include "util.h"

#define UI_ELEM_TO_TAG_APP(elem) \
	container_of((elem), struct tag_app, ui_element)

esp_err_t tag_app_render_render(struct ui_element_render* render, struct ui_element* elem, struct display* disp, TickType_t last_animate_tick, TickType_t ticks) {
	struct tag_app* app = UI_ELEM_TO_TAG_APP(elem);

	return display_text_display(disp, app->display_string);
}

struct ui_element_render_ops tag_app_render_text_ops = {
	.render = tag_app_render_render,
};

struct ui_element_render tag_app_render_text = {
	.flags = {
		.text = 1,
	},
	.ops = &tag_app_render_text_ops
};
