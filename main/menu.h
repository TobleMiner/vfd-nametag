#ifndef _MENU_H_
#define _MENU_H_

#include <stdbool.h>

#include "esp_err.h"

struct menu;

struct menu {
	char* name; // Must be NULL for top level entry
	void* val;

	struct menu* entries; // List of entries, terminate with entry where name, entries and select are set to NULL
	
	esp_err_t (*select)(struct menu* entry); // Set to NULL if this is not a leaf

	struct menu* parent; // DO NOT TOUCH! Automagically initialized through menu_init
};

struct menu_state {
	struct menu* current_entry;
};

#define menu_current_entry(state) ((state)->current_entry)
#define menu_entry_name(entry) ((entry)->name)
#define menu_current_name(state) (menu_entry_name(menu_current_entry(state)))
#define menu_can_descend(state) (!!(state)->current_entry->entries)
#define menu_can_ascend(state) (!!(state)->current_entry->parent->name)

esp_err_t menu_init(struct menu* menu, struct menu_state* state);
esp_err_t menu_descend(struct menu_state* state);
esp_err_t menu_ascend(struct menu_state* state);
esp_err_t menu_next(struct menu_state* state);
esp_err_t menu_prev(struct menu_state* state);


#endif
