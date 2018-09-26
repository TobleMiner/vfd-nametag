#ifndef _MENU_RENDER_H_
#define _MENU_RENDER_H_

#include "ui.h"

struct menu_render {
	struct ui_element_render ui_render;
};

esp_err_t menu_render_alloc(struct menu_render** retval);

#endif
