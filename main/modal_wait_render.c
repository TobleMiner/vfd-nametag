#include <stdlib.h>
#include <string.h>

#include "esp_err.h"

#include "modal_wait_render.h"
#include "display.h"

esp_err_t modal_wait_text_render(struct ui_element_render* render, struct ui_element* elem, struct display* disp, TickType_t last_animation_tick, TickType_t ticks) {
	esp_err_t err;
	size_t display_width, text_len, len, num_spinner_chars;
	char* buff;
	struct modal_wait* mod = UI_ELEMENT_TO_MODAL_WAIT(elem);

	if((err = display_text_get_width(disp, &display_width))) {
		goto fail;
	}

	buff = calloc(1, display_width + 1);
	if(!buff) {
		err = ESP_ERR_NO_MEM;
		goto fail;
	}

	memset(buff, ' ', display_width);

	text_len = strlen(mod->text);
	len = min(text_len, display_width);

	memcpy(buff, mod->text, len);

	num_spinner_chars = display_text_get_spinner_char_cnt(disp);
	buff[display_width - 1] = display_text_get_spinner_char(disp, ticks / (MODAL_WAIT_SPINNER_INTERVAL / portTICK_PERIOD_MS / num_spinner_chars) % num_spinner_chars);

	if((err = display_text_display(disp, buff))) {
		goto fail_alloc;
	}

fail_alloc:
	free(buff);
fail:
	return err;
}

struct ui_element_render_ops modal_wait_render_text_ops = {
	.render = modal_wait_text_render,
};

struct ui_element_render modal_wait_render_text = {
	.flags = {
		.text = 1,
	},
	.ops = &modal_wait_render_text_ops,
};
