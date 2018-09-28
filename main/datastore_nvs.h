#ifndef _DATASTORE_NVS_H_
#define _DATASTORE_NVS_H_

#include <nvs.h>

#include "datastore.h"

struct datastore_nvs {
	struct datastore ds;
	nvs_handle nvs_handle;

	struct datastore* cache;
};

esp_err_t datastore_nvs_alloc(struct datastore** retval, struct datastore_kvpair_default* defaults, size_t len);
esp_err_t datastore_nvs_load(struct datastore* ds, void** value, const char* key, int datatype);
esp_err_t datastore_nvs_load_inplace(struct datastore* ds, void* value, size_t len, const char* key, int datatype);
esp_err_t datastore_nvs_store(struct datastore* ds, void* value, const char* key, int datatype);

#endif
