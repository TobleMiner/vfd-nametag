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

