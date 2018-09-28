#include "menu.h"
#include "value_editor.h"

#define menu_entry_is_delimiter(entry) (!((entry)->name || (entry)->entries || (entry)->select_cb))
#define menu_entry_is_last_child(entry) (menu_entry_is_delimiter(entry + 1))
#define menu_entry_is_first_child(entry) ((entry) == &(entry)->parent->entries[0])

#define STATE_TO_MENU(state) \
	container_of((state), struct menu, state);

#define UI_ELEMENT_TO_MENU(elem) \
	container_of((elem), struct menu, ui_element);

extern struct ui_element_render menu_render_text;

static esp_err_t menu_action_performed(struct ui* ui, struct ui_element* elem, userio_action action) {
	struct menu* menu = UI_ELEMENT_TO_MENU(elem);
	struct menu_state* state = &menu->state;
	switch(action) {
		case USERIO_ACTION_NEXT:
			return menu_next(state);
		case USERIO_ACTION_PREV:
			return menu_prev(state);
		case USERIO_ACTION_SELECT:
			return menu_descend(ui, state);
		case USERIO_ACTION_BACK:
			return menu_ascend(state);
	}
	return ESP_ERR_NOT_SUPPORTED;
}

struct ui_element_ops menu_ops = {
	.action_performed = menu_action_performed,
};

struct ui_element_def menu_def = {
	.renders = {
		&menu_render_text,
		NULL,
	},
	.ops = &menu_ops,
};


esp_err_t menu_alloc(struct menu** retval, struct ui* ui, struct menu_entry* root, struct datastore* ds_volatile, struct datastore* ds_persistent, menu_leave_cb leave_cb, void* priv) {
	esp_err_t err;
	struct menu_entry* parent, *cursor;
	struct menu* menu = calloc(1, sizeof(struct menu));
	if(!menu) {
		err = ESP_ERR_NO_MEM;
		goto fail;
	}

	menu->ds_volatile = ds_volatile;
	menu->ds_persistent = ds_persistent;

	menu->leave_cb = leave_cb;
	menu->leave_cb_priv = priv;

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

	ui_element_init(&menu->ui_element, &menu_def);

	*retval = menu;
	return ESP_OK;

fail_alloc:
	free(menu);
fail:
	return err;
}

struct datastore* menu_get_datastore(struct menu* menu, struct menu_entry* entry) {
	if(entry->entry_data.flags.persistent) {
		return menu->ds_persistent;
	}
	return menu->ds_volatile;
}

static esp_err_t menu_select_entry_semantic(struct menu* menu, struct menu_entry* entry) {
	esp_err_t err;
	struct datastore* ds = menu_get_datastore(menu, entry);

	switch(entry->entry_data.semantic_type) {
		case MENU_ENTRY_TYPE_ON_OFF: {
			int8_t val;
			if((err = -datastore_load_inplace(ds, &val, sizeof(val), entry->entry_data.key, entry->entry_data.datatype)) > 0) {
				goto fail;
			}
			val = !val;
			if((err = datastore_store(ds, &val, entry->entry_data.key, entry->entry_data.datatype))) {
				goto fail;
			}
			break;
		}
	}
	return ESP_OK;

fail:
	return err;
}

static esp_err_t menu_start_editor(struct ui* ui, struct menu* menu, struct menu_entry* entry) {
	esp_err_t err;
	struct value_editor_config conf;
	struct value_editor* editor;
	struct datastore* ds = menu_get_datastore(menu, entry);

	conf.min = entry->entry_data.min;
	conf.max = entry->entry_data.max;
	conf.flags.readonly = entry->entry_data.flags.readonly;

	if((err = value_editor_alloc(&editor, &menu->ui_element, entry->entry_data.key, entry->entry_data.datatype, ds, &conf))) {
		return err;
	}

	ui_add_element(&editor->ui_element, ui);
	ui_set_active_element(ui, &editor->ui_element);

	return ESP_OK;
}

esp_err_t menu_descend(struct ui* ui, struct menu_state* state) {
	esp_err_t err;
	struct menu* menu;
	struct menu_entry* entry = state->current_entry;

	if(menu_can_descend(state)) {
		state->current_entry = &entry->entries[0];
		return ESP_OK;
	}

	menu = STATE_TO_MENU(state);
	if(entry->entry_data.semantic_type) {
		if((err = menu_select_entry_semantic(menu, entry))) {
			return err;
		}
	}

	if(entry->select_cb) {
		return state->current_entry->select_cb(menu, state->current_entry, NULL);
	} else if(!(entry->entry_data.semantic_type || entry->entry_data.flags.suppress_editor)) {
		printf("Starting editor\n");
		return menu_start_editor(ui, menu, entry);
	}

	return ESP_OK;
}

esp_err_t menu_ascend(struct menu_state* state) {
	struct menu* menu;

	if(menu_can_ascend(state)) {
		state->current_entry = state->current_entry->parent;
		return ESP_OK;
	}

	menu = STATE_TO_MENU(state);
	if(menu->leave_cb) {
		return menu->leave_cb(menu->leave_cb_priv);
	}

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
