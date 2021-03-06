#include <stdlib.h>
#include <string.h>

#include "esp_err.h"

#include "util.h"
#include "list.h"
#include "datastore.h"

extern struct datastore_def datastore_mem;
extern struct datastore_def datastore_nvs;

struct datastore_def* datastore_defs[] = {
	&datastore_mem,
	&datastore_nvs
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

	dst->key = strdup(src->key);
	if(!dst->key) {
		err = ESP_ERR_NO_MEM;
		goto fail;
	}

	if(src->value) {
		if((err = datastore_clone_value(&dst->value, src->value, src->datatype))) {
			goto fail_key_alloc;
		}
	}

	return ESP_OK;
	
fail_key_alloc:
	free(dst->key);
fail:
	return err;
}

esp_err_t datastore_init(struct datastore* ds, struct datastore_def* def, struct datastore_kvpair_default* defaults, size_t len) {
	esp_err_t err;

	ds->def = def;

	if(len) {
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
	} else {
		ds->num_defaults = 0;
	}

	return ESP_OK;

fail_defaults:
	datastore_free(ds);
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
}

static esp_err_t datastore_load_default(struct datastore* ds, void** value, const char* key, int datatype) {
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
	return ESP_ERR_NOT_FOUND;
}

esp_err_t datastore_load(struct datastore* ds, void** value, const char* key, int datatype) {
	esp_err_t err;

	err = ds->def->ops->load(ds, value, key, datatype);
	if((err && err != ESP_ERR_NOT_FOUND) || *value) {
		return err;
	}

	if((err = datastore_load_default(ds, value, key, datatype))) {
		return err;
	}

	if((err = datastore_store(ds, *value, key, datatype))) {
		free(*value);
		*value = NULL;
	}

	return err;
}

ssize_t datastore_load_inplace(struct datastore* ds, void* value, size_t len, const char* key, int datatype) {
	void* data;
	ssize_t datalen;

	if(ds->def->ops->load_inplace) {
		datalen = ds->def->ops->load_inplace(ds, value, len, key, datatype);
		if((datalen < 0 && (datalen != -ESP_ERR_NOT_FOUND)) || datalen > 0) {
			return datalen;
		}
	}

	datalen = datastore_load(ds, &data, key, datatype);
	if(datalen) {
		// Beware of inverted error logic!
		return -datalen;
	}

	len = min(datatype_get_size(datatype, data), len);
	memcpy(value, data, len);
	free(data);
	return len;
}
