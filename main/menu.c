#include "menu.h"

#define menu_entry_is_delimiter(entry) (!((entry)->name || (entry)->entries || (entry)->select_cb))
#define menu_entry_is_last_child(entry) (menu_entry_is_delimiter(entry + 1))
#define menu_entry_is_first_child(entry) ((entry) == &(entry)->parent->entries[0])

esp_err_t menu_alloc(struct menu** retval, struct menu_entry* root, struct datastore* ds) {
	esp_err_t err;
	struct menu_entry* parent, *cursor;
	struct menu* menu = calloc(1, sizeof(struct menu));
	if(!menu) {
		err = ESP_ERR_NO_MEM;
		goto fail;
	}

	menu->datastore = ds;

	if(root->name || !root->entries) {
		// Not a toplevel menu
		err = ESP_ERR_INVALID_ARG;
		goto fail_alloc;
	}
	root->parent = NULL;
	parent = root;
	cursor = &root->entries[0];
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

	menu->state.current_entry = &root->entries[0];

	*retval = menu;
	return ESP_OK;

fail_alloc:
	free(menu);
fail:
	return err;
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
		state->current_entry = &(state->current_entry - 1)->parent->entries[0];
	}
	return ESP_OK;
}

esp_err_t menu_prev(struct menu_state* state) {
	if(menu_entry_is_first_child(state->current_entry)) {
		while(!menu_entry_is_delimiter(state->current_entry)) {
			state->current_entry++;
		}
	}
	state->current_entry--;
	return ESP_OK;
}

