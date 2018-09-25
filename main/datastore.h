#ifndef _DATASTORE_H_
#define _DATASTORE_H_

#include "list.h"
#include "datatypes.h"

struct datastore_kvpair {
	char* key;
	void* value;
	int datatype;
};

typedef esp_err_t (*datastore_default_value_cb)(void** value, char* key, int datatype, void* priv);

/* Do NOT pass values that must be freed as default values through the kvpair member!
   Use the default_cb instead! 
 */
struct datastore_kvpair_default {
	struct datastore_kvpair kvpair;
	void* priv;
	datastore_default_value_cb default_cb;
};

struct datastore;

struct datastore_ops {
	esp_err_t alloc(struct datastore** retval, struct datastore_kvpair_default* defaults, size_t len);
	void free(struct datastore* ds);

	/* Values loaded from a datastore are malloced and MUST be freed! */
	esp_err_t load(struct datstore* ds, void** value, char* key, int datatype);
	/* Supports only fixed length datatypes and NUL-terminated strings */
	esp_err_t store(struct datstore* ds, void* value, char* key, int datatype);
};

/* Do not use any of the following ops from outside the internal datastore code! External use
   may kill little kittens, cause global thermonuclear war or reappoint George W. Bush!
 */
struct datastore_priv_ops {
	/* Check if value for key is available in storage backend */
	esp_err_t _has_key(struct datastore* ds, char* key);
};

typedef uint8_t datstore_flags;

#define DATASTORE_FLAG_PERSISTENT 0b01;

typedef uint8_t datstore_priv_flags;

#define DATASTORE_FLAG_CACHED 0b01;

struct datastore_def {
	uint8_t flags;

	uint8_t priv_flags;

	struct datastore_ops* ops;
};

struct datastore_entry {
	struct list_head list;
	struct datastore_kvpair data;
};

struct datastore {
	struct datastore_def* def;

	/* List of struct datastore_entry */
	struct list_head cache;

	struct datastore_kvpair_default* defaults;
	size_t len_defaults;
};

#endif