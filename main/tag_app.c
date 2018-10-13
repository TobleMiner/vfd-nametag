#include <stdlib.h>
#include <string.h>

#include "tag_app.h"
#include "list.h"

extern struct ui_element_render tag_app_render_text;

#define UI_ELEM_TO_TAG_APP(elem) \
	container_of((elem), struct tag_app, ui_element)

static esp_err_t tag_app_action_performed(struct ui* ui, struct ui_element* elem, userio_action action) {
	struct tag_app* app = UI_ELEM_TO_TAG_APP(elem);

	switch(action) {
		case USERIO_ACTION_BACK:
			ui_set_active_element(ui, &app->menu->ui_element);
	}
	return ESP_OK;
}

struct ui_element_ops tag_app_ops = {
	.action_performed = tag_app_action_performed,
};

struct ui_element_def tag_app_def = {
	.renders = {
		&tag_app_render_text,
		NULL,
	},
	.ops = &tag_app_ops,
};

esp_err_t tag_app_alloc(struct tag_app** retval, struct menu* menu) {
	esp_err_t err;
	struct tag_app* app = calloc(1, sizeof(struct tag_app));
	if(!app) {
		err = ESP_ERR_NO_MEM;
		goto fail;
	}

	ui_element_init(&app->ui_element, &tag_app_def);

	app->menu = menu;
	if((err = tag_app_set_string(app, "<unset>"))) {
		err = ESP_ERR_NO_MEM;
		goto fail_alloc;
	}

	*retval = app;
	return ESP_OK;

fail_alloc:
	free(app);
fail:
	return err;
}

esp_err_t tag_app_set_string(struct tag_app* app, char* str) {
	esp_err_t err = ESP_OK;
	char* oldstr = app->display_string;
	char* heapstr = strdup(str);
	if(!heapstr) {
		err = ESP_ERR_NO_MEM;
		goto fail;
	}

	app->display_string = heapstr;
	free(oldstr);
	
fail:
	return err;
}

esp_err_t tag_app_update(struct tag_app* app) {
	return ESP_OK;
}
