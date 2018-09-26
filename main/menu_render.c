#include "menu.h"
#include "menu_render.h"

#define UI_ELEM_TO_MENU(elem) \
	container_of((elem), struct menu, ui_element)

esp_err_t menu_render_render(struct ui_element_render* render, struct ui_element* elem, struct display* disp) {
	struct menu* menu = UI_ELEM_TO_MENU(elem);
	if(!menu_entry_is_leaf(menu->state.current_entry)) {
		return display_text_display(disp, menu_current_name(&menu->state));
	}

// TODO: Content aware rendering
	return display_text_display(disp, menu_current_name(&menu->state));

//	return ESP_OK;
}

struct ui_element_render_ops menu_render_ops = {
	.render = menu_render_render,
};

esp_err_t menu_render_alloc(struct menu_render** retval) {
	esp_err_t err;

	struct menu_render* render = calloc(1, sizeof(struct menu_render));
	if(!render) {
		err = ESP_ERR_NO_MEM;
		goto fail;
	}

	ui_element_render_init(&render->ui_render, &menu_render_ops);

	render->ui_render.flags.text = 1;

	*retval = render;
	return ESP_OK;

fail:
	return err;
}
