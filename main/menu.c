#include "menu.h"

#define menu_entry_is_delimiter(entry) (!((entry)->name || (entry)->entries || (entry)->select))
#define menu_entry_is_first_child(entry) ((entry) == &(entry)->parent->entries[0])

esp_err_t menu_init(struct menu* menu, struct menu_state* state) {
	struct menu* parent, *cursor;

	if(menu->name || !menu->entries) {
		// Not a toplevel menu
		return ESP_ERR_INVALID_ARG;
	}
	menu->parent = NULL;
	parent = menu;
	cursor = &menu->entries[0];
	while(parent) {
		cursor->parent = parent;
		if(cursor->entries) {
			parent = cursor;
			cursor = &cursor->entries[0];
		} else {
			cursor++;
		}
		while(parent && menu_entry_is_delimiter(cursor)) {
			cursor = parent;
			parent = parent->parent;
			cursor++;
		}
	}

	state->current_entry = &menu->entries[0];
	return ESP_OK;
}

esp_err_t menu_descend(struct menu_state* state) {
	if(!menu_can_descend(state)) {
		return ESP_ERR_INVALID_ARG;
	}
	state->current_entry = &state->current_entry->entries[0];
	return ESP_OK;
}

esp_err_t menu_ascend(struct menu_state* state) {
	if(!menu_can_ascend(state)) {
		return ESP_ERR_INVALID_ARG;
	}
	state->current_entry = state->current_entry->parent;
	return ESP_OK;
}

esp_err_t menu_next(struct menu_state* state) {
	state->current_entry++;
	if(menu_entry_is_delimiter(state->current_entry)) {
		state->current_entry = &state->current_entry->parent->entries[0];
	}
	return ESP_OK;
}

esp_err_t menu_prev(struct menu_state* state) {
	if(menu_entry_is_first_child(state->current_entry)) {
		while(!menu_entry_is_delimiter(++state->current_entry));
	}
	state->current_entry--;
	return ESP_OK;
}

