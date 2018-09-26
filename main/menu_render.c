
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_err.h"

#include "menu.h"
#include "menu_render.h"
#include "util.h"

#define UI_ELEM_TO_MENU(elem) \
	container_of((elem), struct menu, ui_element)

esp_err_t menu_render_render(struct ui_element_render* render, struct ui_element* elem, struct display* disp) {
	esp_err_t err;
	struct menu* menu = UI_ELEM_TO_MENU(elem);
	struct menu_entry* entry = menu->state.current_entry;
	char* name = menu_current_name(&menu->state);

	if(menu_entry_is_leaf(entry) && entry->entry_data.semantic_type) {
		char* render, *name_end;
		size_t name_len = strlen(name);
		size_t display_width;
		size_t leftover_space;
		struct datastore* ds;

		if((err = display_text_get_width(disp, &display_width))) {
			goto fail;
		}

		render = calloc(1, display_width + 1);
		if(!render) {
			err = ESP_ERR_NO_MEM;
			goto fail;
		}

		memcpy(render, name, name_len);
		name_end = render + name_len;
		leftover_space = display_width - name_len;

		ds = menu->ds_volatile;
		if(entry->entry_data.flags.persistent) {
			ds = menu->ds_persistent;
		}

		switch(entry->entry_data.semantic_type) {
			case MENU_ENTRY_TYPE_ON_OFF: {
				int8_t* val;
				char* strval;
				size_t vallen;
				if(datastore_load(ds, &val, entry->entry_data.key, entry->entry_data.datatype)) {
					goto fail_semantic;
				}
				if(!val) {
					err = ESP_ERR_NOT_FOUND;
					goto fail_semantic;
				}
				strval = *val ? "ON" : "OFF";
				vallen = strlen(strval);
				while(leftover_space-- > vallen) {
					*name_end++ = ' ';
				}
				snprintf(name_end, vallen + 1, strval);
				free(val);
				break;
			}
			default:
				strcpy(render, "<INVAL TYPE>");
		}
	
		err = display_text_display(disp, render);
fail_semantic:
		free(render);
		return err;
	}
	return display_text_display(disp, name);


fail:
	return err;
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
