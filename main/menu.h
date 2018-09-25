#ifndef _MENU_H_
#define _MENU_H_

#include <stdbool.h>

#include "esp_err.h"

#include "datastore.h"

enum {
	MENU_ENTRY_CUSTOM,
	MENU_ENTRY_ON_OFF,
	MENU_ENTRY_INT,
	MENU_ENTRY_STRING,

	_MENU_ENTRY_MAX
};

struct menu_entry_data;

struct menu_entry_data {
	int type;
	int display_type;
	const char* key;

	void* (*get_default_value)(struct menu_entry_data* entry, struct menu* menu);

	int64_t min;
	int64_t max;
};


struct menu_entry;

typedef esp_err_t (*menu_select_cb)(struct menu* entry, void* priv);

typedef char* (*menu_name_cb)(struct menu* entry, void* priv);

struct menu_entry {
	char* name; // Must be NULL for top level entry

	struct menu_entry* entries; // List of entries, terminate with entry where name, entries and select are set to NULL

	struct menu_entry* parent; // DO NOT TOUCH! Automagically initialized through menu_init

	struct menu_entry_data entry_data;
	
	menu_select_cb select_cb;
};

struct menu_state {
	struct menu* current_entry;
};

struct menu {
	struct menu_entry* root;
	struct datastore* datastore;
	struct menu_state state;
};

inline char* menu_entry_name(struct menu_entry* entry) {
	if(entry->name) {
		return entry->name;
	}
	if(entry->name_cb) {
		return entry->name_cb(entry, entry->name_cb_priv);
	}
	return NULL;
}

#define menu_current_entry(state) ((state)->current_entry)
#define menu_current_name(state) (menu_entry_name(menu_current_entry(state)))
#define menu_can_descend(state) (!!(state)->current_entry->entries)
#define menu_can_ascend(state) (!!(state)->current_entry->parent->name)

esp_err_t menu_alloc(struct menu** retval, struct menu_entry* root, struct menu_state* state, struct datastore* store);
esp_err_t menu_descend(struct menu_state* state);
esp_err_t menu_ascend(struct menu_state* state);
esp_err_t menu_next(struct menu_state* state);
esp_err_t menu_prev(struct menu_state* state);

#endif
