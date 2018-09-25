#include <stdlib.h>
#include <string.h>

#include "esp_err.h"

#include "util.h"
#include "list.h"
#include "datastore.h"

extern struct datastore_def datastore_mem;

struct datastore_def* datastore_defs[] = {
	&datastore_mem
};

esp_err_t datastore_alloc(struct datastore** retval, datastore_flags flags, struct datastore_kvpair_default* defaults, size_t len) {
	unsigned int i;
	for(i = 0; i < ARRAY_LEN(datastore_defs); i++) {
		struct datastore_def* def = datastore_defs[i];
		if(def->flags != flags) {
			continue;
		}
		return def->ops->alloc(retval, defaults, len);
	}

	return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t datastore_clone_value(void** retval, void* src, int datatype) {
	esp_err_t err;
	void* dst;
	ssize_t len;

	len = datatype_get_size(datatype, src);
	if(len < 0) {
		err = ESP_ERR_INVALID_ARG;
		goto fail;
	}
	dst = calloc(1, len);
	if(!dst) {
		err = ESP_ERR_NO_MEM;
		goto fail;
	}
	memcpy(dst, src, len);

	*retval = dst;
	return ESP_OK;

fail:
	return err;
}

esp_err_t datastore_copy_kvpair(struct datastore_kvpair* dst, struct datastore_kvpair* src) {
	esp_err_t err;

	dst->datatype = src->datatype;

	if((err = datastore_clone_value(&dst->value, src->value, src->datatype))) {
		goto fail;
	}

	dst->key = strdup(src->key);
	if(!dst->key) {
		err = ESP_ERR_NO_MEM;
		goto fail_value_alloc;
	}

	return ESP_OK;
	
fail_value_alloc:
	free(dst->value);
fail:
	return err;
}

esp_err_t datastore_init(struct datastore* ds, struct datastore_def* def, struct datastore_kvpair_default* defaults, size_t len) {
	esp_err_t err;

	ds->def = def;
	INIT_LIST_HEAD(ds->cache);

	ds->defaults = calloc(len, sizeof(struct datastore_kvpair_default));
	if(!ds->defaults) {
		err = ESP_ERR_NO_MEM;
		goto fail;
	}

	for(ds->num_defaults = 0; ds->num_defaults < len; ds->num_defaults++) {
		struct datastore_kvpair_default* src, *dst;
		dst = &ds->defaults[ds->num_defaults];
		src = &defaults[ds->num_defaults];

		dst->priv = src->priv;
		dst->default_cb = src->default_cb;

		if((err = datastore_copy_kvpair(&dst->kvpair, &src->kvpair))) {
			goto fail_defaults;
		}
	}

	return ESP_OK;

fail_defaults:
	while(ds->num_defaults-- > 0) {
		struct datastore_kvpair* kvpair = &ds->defaults[ds->num_defaults].kvpair;
		free(kvpair->key);
		free(kvpair->value);	
	}
	free(ds->defaults);
fail:
	return err;
}

void datastore_free(struct datastore* ds) {
	while(ds->num_defaults-- > 0) {
		struct datastore_kvpair* kvpair = &ds->defaults[ds->num_defaults].kvpair;
		free(kvpair->key);
		free(kvpair->value);	
	}
	free(ds->defaults);

	// TODO free cache contents
}

static esp_err_t datastore_load_default(struct datastore* ds, void** value, char* key, int datatype) {
	size_t i;

	for(i = 0; i < ds->num_defaults; i++) {
		struct datastore_kvpair_default* def = &ds->defaults[i];
		if(strcmp(key, def->kvpair.key)) {
			continue;
		}
		if(def->kvpair.datatype != datatype) {
			return ESP_ERR_INVALID_STATE;
		}
		if(def->kvpair.value) {
			return datastore_clone_value(value, def->kvpair.value, datatype);
		}

		return def->default_cb(value, key, datatype, def->priv);
	}

	*value = NULL;
	return ESP_OK;
}

esp_err_t datastore_load(struct datastore* ds, void** value, char* key, int datatype) {
	esp_err_t err;

	err = ds->def->ops->load(ds, value, key, datatype);
	if(err || *value) {
		return err;
	}

	return datastore_load_default(ds, value, key, datatype);	
}
