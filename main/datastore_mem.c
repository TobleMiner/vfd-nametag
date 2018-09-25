#include "datastore_mem.h"

#define DS_TO_DS_MEM(ds) container_of((ds), struct datastore_mem, ds)

static esp_err_t datastore_mem_alloc(struct datastore** retval, struct datastore_kvpair_default* defaults, size_t len) {
	esp_err_t err;
	struct datastore_mem* ds_mem = calloc(1, sizeof(struct datastore_mem));
	if(!ds_mem) {
		err = ESP_ERR_NO_MEM;
		goto fail;
	}

	INIT_LIST_HEAD(ds_mem->storage);

	if((err = datastore_init(&ds_mem->ds, defaults, len))) {
		goto fail_alloc;
	}

	*retval = &ds_mem->ds;
	return ESP_OK;

fail_alloc:
	free(ds_mem);
fail:
	return err;
}

static esp_err_t datastore_mem_load(struct datastore* ds, void** value, char* key, int datatype) {
	struct datastore_mem* ds_mem = DS_TO_DS_MEM(ds);
	struct list_head* cursor;

	LIST_FOR_EACH(cursor, &ds_mem->storage) {
		struct datastore_mem_kvpair* mem_kvpair = LIST_ENTRY(cursor, struct datastore_mem_kvpair, list);
		struct datastore_kvpair* kvpair = &mem_kvpair->kvpair;

		if(strcmp(kvpair->key, key)) {
			continue;
		}

		if(kvpair->datatype != datatype) {
			return ESP_ERR_INVALID_STATE;
		}

		return datastore_clone_value(value, kvpair->value, datatype);
	}

	*value = NULL;
	return ESP_OK;
}

static esp_err_t datastore_mem_store(struct datastore* ds, void* value, char* key, int datatype) {
	esp_err_t err;
	struct datastore_mem_kvpair* mem_kvpair;
	struct datastore_kvpair* kvpair;
	struct datastore_mem* ds_mem = DS_TO_DS_MEM(ds);
	struct list_head* cursor;

	LIST_FOR_EACH(cursor, &ds_mem->storage) {
		mem_kvpair = LIST_ENTRY(cursor, struct datastore_mem_kvpair, list);
		kvpair = &mem_kvpair->kvpair;
		void* tmpptr;

		if(strcmp(kvpair->key, key)) {
			continue;
		}

		if(kvpair->datatype != datatype) {
			return ESP_ERR_INVALID_STATE;
		}

		if((err = datastore_clone_value(&tmpptr, value, datatype))) {
			return err;
		}

		// Free old value only if data cloning was successful
		free(kvpair->value);
		kvpair->value = tmpptr;

		return ESP_OK;
	}

	// There is no entry for this key, create a new one
	mem_kvpair = calloc(1, sizeof(struct datastore_mem_kvpair));
	if(!mem_kvpair) {
		goto fail;
	}

	return ESP_OK;
fail:
	return err;
}

struct datastore_ops datastore_mem_ops = {
	.alloc = datastore_mem_alloc,
};

struct datastore_def datastore_mem = {
	.flags = 0,
	.priv_flags = 0,
	.ops = &datastore_mem_ops,
};