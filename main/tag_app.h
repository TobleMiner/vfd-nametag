#ifndef _TAG_APP_H_
#define _TAG_APP_H_

#include "esp_err.h"

#include "menu.h"
#include "ui.h"

struct tag_app {
	struct ui_element ui_element;
	struct menu* menu;
	char* display_string;
};

esp_err_t tag_app_alloc(struct tag_app** retval, struct menu* menu);
esp_err_t tag_app_update(struct tag_app* app);
esp_err_t tag_app_set_string(struct tag_app* app, char* str);

#endif
