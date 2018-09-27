#ifndef _DATASTORE_MEM_H_
#define _DATASTORE_MEM_H_

#include "datastore.h"

struct datastore_mem_kvpair {
	struct datastore_kvpair kvpair;
	struct list_head list;
};

struct datastore_mem {
	struct datastore ds;

	/* List of struct datastore_mem_kvpair */
	struct list_head storage;
};

esp_err_t datastore_mem_alloc(struct datastore** retval, struct datastore_kvpair_default* defaults, size_t len);
esp_err_t datastore_mem_load(struct datastore* ds, void** value, const char* key, int datatype);
esp_err_t datastore_mem_load_inplace(struct datastore* ds, void* value, size_t len, const char* key, int datatype);
esp_err_t datastore_mem_store(struct datastore* ds, void* value, const char* key, int datatype);

#endif
