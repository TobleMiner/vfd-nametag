#include <stdlib.h>
#include <string.h>

#include "esp_err.h"

#include "value_editor_render.h"
#include "display.h"

#define VALUE_EDITOR_FMT_INT "%lld"

esp_err_t value_editor_text_render_string(struct value_editor* editor, struct display* disp) {
	if(!editor->conf.flags.readonly) {
		display_text_display(disp, "<WR NOT SUP>");
		return ESP_ERR_NOT_SUPPORTED;
	}

	return display_text_display(disp, (char*)editor->value);
}

esp_err_t value_editor_text_render_int(struct value_editor* editor, struct display* disp) {
	esp_err_t err;
	int64_t val;
	size_t strlen, display_width;
	char* render, *str;

	if(!editor->conf.flags.readonly) {
		display_text_display(disp, "<WR NOT SUP>");
		return ESP_ERR_NOT_SUPPORTED;
	}
	switch(editor->datatype) {
		case DATATYPE_INT8:
			val = *((int8_t*)editor->value);
			break;
		case DATATYPE_INT16:
			val = *((int16_t*)editor->value);
			break;
		case DATATYPE_INT32:
			val = *((int32_t*)editor->value);
			break;
		case DATATYPE_INT64:
			val = *((int64_t*)editor->value);
			break;
		default:
			return ESP_ERR_INVALID_ARG;
	}
	strlen = snprintf(NULL, 0, VALUE_EDITOR_FMT_INT, val);

	if((err = display_text_get_width(disp, &display_width))) {
		return err;
	}

	render = calloc(1, display_width + 1);
	if(!render) {
		err = ESP_ERR_NO_MEM;
		goto fail;
	}

	memset(render, ' ', display_width);

	str = render + display_width / 2;
	str -= strlen / 2;
	if(str < render) {
		str = render;
	}

	snprintf(str, display_width - (str - render) + 1, VALUE_EDITOR_FMT_INT, val);

	err = display_text_display(disp, render);

	free(render);
fail:
	return err;
}

esp_err_t value_editor_text_render(struct ui_element_render* render, struct ui_element* elem, struct display* disp, TickType_t last_animate_tick, TickType_t ticks) {
	struct value_editor* editor = UI_ELEMENT_TO_VALUE_EDITOR(elem);
	switch(editor->datatype) {
		case DATATYPE_STRING:
			return value_editor_text_render_string(editor, disp);
		case DATATYPE_INT8:
		case DATATYPE_INT16:
		case DATATYPE_INT32:
		case DATATYPE_INT64:
			return value_editor_text_render_int(editor, disp);
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
