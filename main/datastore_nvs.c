#include "datastore_nvs.h"
#include "datastore_mem.h"

#include <stdlib.h>
#include <string.h>

#define DS_TO_DS_NVS(ds) container_of((ds), struct datastore_nvs, ds)

struct datastore_ops datastore_nvs_ops = {
	.alloc = datastore_nvs_alloc,
	.load = datastore_nvs_load,
	.store = datastore_nvs_store,
};

struct datastore_def datastore_nvs = {
	.flags = DATASTORE_FLAG_PERSISTENT,
	.ops = &datastore_nvs_ops,
};

esp_err_t datastore_nvs_alloc(struct datastore** retval, struct datastore_kvpair_default* defaults, size_t len) {
	esp_err_t err;
	struct datastore_nvs* ds_nvs = calloc(1, sizeof(struct datastore_nvs));
	if(!ds_nvs) {
		err = ESP_ERR_NO_MEM;
		goto fail;
	}

	if((err = datastore_init(&ds_nvs->ds, &datastore_nvs, defaults, len))) {
		goto fail_alloc;
	}

	if((err = nvs_open("datastore_nvs", NVS_READWRITE, &ds_nvs->nvs_handle))) {
		goto fail_ds_init;
	}

	if((err = datastore_mem_alloc(&ds_nvs->cache, NULL, 0))) {
		goto fail_nvs_open;
	}

	*retval = &ds_nvs->ds;
	return ESP_OK;

fail_nvs_open:
	nvs_close(ds_nvs->nvs_handle);
fail_ds_init:
	datastore_free(&ds_nvs->ds);
fail_alloc:
	free(ds_nvs);
fail:
	return err;
}

esp_err_t datastore_nvs_xlate_err(esp_err_t err) {
	switch(err) {
		case ESP_ERR_NVS_NOT_FOUND:
			err = ESP_ERR_NOT_FOUND;
	}
	return err;
}

esp_err_t datastore_nvs_load(struct datastore* ds, void** value, const char* key, int datatype) {
	esp_err_t err;
	struct datastore_nvs* ds_nvs = DS_TO_DS_NVS(ds);

	if(!(err = datastore_load(ds_nvs->cache, value, key, datatype))) {
		return ESP_OK;
	}

	if(err != ESP_ERR_NOT_FOUND) {
		return err;
	}

	switch(datatype) {
		case DATATYPE_STRING: {
			size_t store_len;
			if((err = nvs_get_str(ds_nvs->nvs_handle, key, NULL, &store_len))) {
				err = datastore_nvs_xlate_err(err);
				goto fail_str;
			}

			char* str = calloc(1, store_len);
			if(!str) {
				err = ESP_ERR_NO_MEM;
				goto fail_str;
			}

			if((err = nvs_get_str(ds_nvs->nvs_handle, key, str, &store_len))) {
				err = datastore_nvs_xlate_err(err);
				goto fail_str_alloc;
			}

			datastore_store(ds_nvs->cache, str, key, datatype);

			*value = str;
			return ESP_OK;
fail_str_alloc:
			free(str);
fail_str:
			return err;
		}
		default: {
			size_t len = datatype_get_size(datatype, NULL);
			size_t store_len;
			if((err = nvs_get_blob(ds_nvs->nvs_handle, key, NULL, &store_len))) {
				err = datastore_nvs_xlate_err(err);
				goto fail_blob;
			}

			if(store_len < len) {
				nvs_erase_key(ds_nvs->nvs_handle, key);
				err = ESP_ERR_NOT_FOUND;
				goto fail_blob;
			}

			uint8_t* blob = calloc(1, store_len);
			if(!blob) {
				err = ESP_ERR_NO_MEM;
				goto fail_blob;
			}

			if((err = nvs_get_blob(ds_nvs->nvs_handle, key, blob, &store_len))) {
				err = datastore_nvs_xlate_err(err);
				goto fail_blob_alloc;
			}

			datastore_store(ds_nvs->cache, blob, key, datatype);

			*value = blob;
			return ESP_OK;

fail_blob_alloc:
			free(blob);
fail_blob:
			return err;
		}
	}
}

esp_err_t datastore_nvs_store(struct datastore* ds, void* value, const char* key, int datatype) {
	esp_err_t err;
	struct datastore_nvs* ds_nvs = DS_TO_DS_NVS(ds);

	switch(datatype) {
		case DATATYPE_STRING:
			if((err = nvs_set_str(ds_nvs->nvs_handle, key, (char*)value))) {
				err = datastore_nvs_xlate_err(err);
				goto fail_str;
			}
			
			break;
fail_str:
			return err;

		default: {
			size_t value_len = datatype_get_size(datatype, NULL);
			if((err = nvs_set_blob(ds_nvs->nvs_handle, key, value, value_len))) {
				err = datastore_nvs_xlate_err(err);
				goto fail_blob;
			}
		}
			break;
fail_blob:
			return err;
	}

	if((err = nvs_commit(ds_nvs->nvs_handle))) {
		return datastore_nvs_xlate_err(err);
	}
	datastore_store(ds_nvs->cache, value, key, datatype);
	return ESP_OK;
}
