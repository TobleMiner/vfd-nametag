#include "esp_err.h"

#include "value_editor_render.h"
#include "display.h"

esp_err_t value_editor_text_render(struct ui_element_render* render, struct ui_element* elem, struct display* disp) {
	struct value_editor* editor = UI_ELEMENT_TO_VALUE_EDITOR(elem);
	switch(editor->datatype) {
		default:
			display_text_display(disp, "<UNSUP TYPE>");
			return ESP_ERR_NOT_SUPPORTED;
	}
	return ESP_OK;
}

struct ui_element_render_ops value_editor_render_text_ops = {
	.render = value_editor_text_render,
};

struct ui_element_render value_editor_render_text = {
	.flags = {
		.text = 1,
	},
	.ops = &value_editor_render_text_ops,
};
