#ifndef _MENU_RENDER_H_
#define _MENU_RENDER_H_

struct menu_entry_render {
	void (*render)(struct menu_render* render, struct menu_state* state);
};

struct menu_render {
	struct display* disp;
	struct menu* menu;
};


#endif
