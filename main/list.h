#include "util.h"


struct list_head;

struct list_head {
	struct list_head *next, *prev;
}

#define INIT_LIST_HEAD(name) \
	(name) = { &(name), &(name) }

#define LIST_FOR_EACH(cursor, list) \
	for((cursor) = (list)->next; (cursor) != (list); (cursor) = cursor->next)

#define LIST_ENTRY(list, type, member) \
	container_of(list, type, member)

#define LIST_APPEND(entry, list) \
	do { \
		list->next->prev = entry; \
		entry->next = list->next; \
		entry->prev = list; \
		list->next = entry; \
	} while(0)
