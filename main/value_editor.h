#ifndef _VALUE_EDITOR_H_
#define _VALUE_EDITOR_H_

#include "util.h"
#include "ui.h"
#include "datastore.h"

#define UI_ELEMENT_TO_VALUE_EDITOR(elem) \
	container_of((elem), struct value_editor, ui_element)

typedef uint8_t value_editor_flag;

struct value_editor_config {
	int64_t min;
	int64_t max;

	struct {
		value_editor_flag readonly:1;
	} flags;
};

struct value_editor {
		struct ui_element ui_element;
		struct ui_element* caller;

	struct value_editor_config conf;

	const char* key;
	int datatype;
	struct datastore* ds;
	void* value;
};

esp_err_t value_editor_alloc(struct value_editor** retval, struct ui_element* caller, const char* key, int datatype, struct datastore* ds, struct value_editor_config* conf);
void value_editor_free(struct value_editor* editor);

#endif
