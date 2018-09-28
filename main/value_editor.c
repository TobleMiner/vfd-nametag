#include <stdlib.h>

#include "esp_err.h"

#include "value_editor.h"
#include "userio.h"
#include "menu.h"
#include "ui.h"

extern struct ui_element_render value_editor_render_text;

static esp_err_t value_editor_action_performed(struct ui* ui, struct ui_element* elem, userio_action action) {
	struct value_editor* editor = UI_ELEMENT_TO_VALUE_EDITOR(elem);
	switch(action) {
		case USERIO_ACTION_BACK:
			ui_set_active_element(ui, editor->caller);
			ui_remove_element(&editor->ui_element);
			value_editor_free(editor);
			break;
		default:
			printf("Value Editor: Unsupported action %u\n", action);
			return ESP_ERR_NOT_SUPPORTED;
	}
	return ESP_OK;
}

struct ui_element_ops value_editor_ops = {
	.action_performed = value_editor_action_performed,
};

struct ui_element_def value_editor_def = {
	.ops = &value_editor_ops,
	.renders = {
		&value_editor_render_text,
		NULL
	},
};

esp_err_t value_editor_alloc(struct value_editor** retval, struct ui_element* caller, const char* key, int datatype, struct datastore* ds, struct value_editor_config* conf) {
	esp_err_t err;
	struct value_editor* editor = calloc(1, sizeof(struct value_editor));
	if(!editor) {
		err = ESP_ERR_NO_MEM;
		goto fail;
	}

	ui_element_init(&editor->ui_element, &value_editor_def);
	editor->caller = caller;

	editor->conf = *conf;

	editor->key = key;
	editor->ds = ds;
	editor->datatype = datatype;
	if((err = datastore_load(ds, &editor->value, key, datatype))) {
		goto fail_alloc;
	}
	
	*retval = editor;
	return ESP_OK;

fail_alloc:
	free(editor);
fail:
	return err;
}

void value_editor_free(struct value_editor* editor) {
	free(editor->value);
	free(editor);
}
