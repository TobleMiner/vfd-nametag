#ifndef _MENU_H_
#define _MENU_H_

#include <stdbool.h>

#include "esp_err.h"

#include "datastore.h"
#include "ui.h"

enum {
	MENU_ENTRY_TYPE_INHERIT = 0,
	MENU_ENTRY_TYPE_ON_OFF,

	_MENU_ENTRY_TYPE_MAX
};

struct menu;

struct menu_entry_data;

typedef uint8_t menu_entry_flag;

struct menu_entry_data {
	int datatype;
	int semantic_type;
	const char* key;

	struct {
		menu_entry_flag readonly:1;
		menu_entry_flag persistent:1;
		menu_entry_flag suppress_editor:1;
	} flags;

	int64_t min;
	int64_t max;
	size_t max_len;
};

typedef esp_err_t (*menu_leave_cb)(void* priv);

struct menu_entry;

typedef esp_err_t (*menu_select_cb)(struct menu* menu, struct menu_entry* entry, void* priv);

struct menu_entry {
	char* name; // Must be NULL for top level entry

	struct menu_entry* entries; // List of entries, terminate with entry where name, entries and select are set to NULL

	struct menu_entry* parent; // DO NOT TOUCH! Automagically initialized through menu_init

	struct menu_entry_data entry_data;
	
	menu_select_cb select_cb;
};

struct menu_state {
	struct menu_entry* current_entry;
};

struct menu {
	struct ui_element ui_element;

	menu_leave_cb leave_cb;
	void* leave_cb_priv;

	struct menu_entry* root;
	struct datastore* ds_volatile;
	struct datastore* ds_persistent;
	struct menu_state state;
};

#define menu_entry_name(entry) (entry)->name
#define menu_entry_is_leaf(entry) ((!(entry)->entries) && (entry)->entry_data.key)
#define menu_current_entry(state) ((state)->current_entry)
#define menu_current_name(state) (menu_entry_name(menu_current_entry(state)))
#define menu_can_descend(state) (!!(state)->current_entry->entries)
#define menu_can_ascend(state) (!!(state)->current_entry->parent->name)

esp_err_t menu_alloc(struct menu** retval, struct ui* ui, struct menu_entry* root, struct datastore* ds_volatile, struct datastore* ds_persistent, menu_leave_cb leave_cb, void* priv);
struct datastore* menu_get_datastore(struct menu* menu, struct menu_entry* entry);
esp_err_t menu_descend(struct ui* ui, struct menu_state* state);
esp_err_t menu_ascend(struct menu_state* state);
esp_err_t menu_next(struct menu_state* state);
esp_err_t menu_prev(struct menu_state* state);

#endif
