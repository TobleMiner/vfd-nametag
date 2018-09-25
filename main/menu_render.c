#include "menu_render.h"

void menu_render_on_off(struct menu_render* render, struct menu_state* state) {
	printf("Render on_off\n");
}

void menu_render_int(struct menu_render* render, struct menu_state* state) {
	printf("Render int\n");
}

void menu_render_string(struct menu_render* render, struct menu_state* state) {
	printf("Render string\n");
}

struct menu_entry_render menu_renders[] = {
	{ },
	{
		.render = menu_render_on_off,
	}
};

esp_err_t menu_render_alloc(struct menu_render** retval, struct display* disp, struct menu* menu) {
	esp_err_t err;

	struct menu_render* render = calloc(1, sizeof(struct menu_render));
	if(!render) {
		err = ESP_ERR_NO_MEM;
		goto fail;
	}

	render->disp = disp;
	render->menu = menu;

	*retval = render;
	return ESP_OK;

fail:
	return err;
}

