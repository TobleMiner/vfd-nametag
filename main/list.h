#ifndef _LIST_H_
#define _LIST_H_

#include <stdint.h>

#include "util.h"

struct list_head;

struct list_head {
	struct list_head *next, *prev;
};

#define INIT_LIST_HEAD(name) \
	(name) = (struct list_head){ &(name), &(name) }

#define LIST_FOR_EACH(cursor, list) \
	for((cursor) = (list)->next; (cursor) != (list); (cursor) = (cursor)->next)

#define LIST_FOR_EACH_SAFE(cursor, n, list) \
	for((cursor) = (list)->next, (n) = (cursor)->next; (cursor) != (list); (cursor) = n, (n) = (cursor)->next)

#define LIST_GET_ENTRY(list, type, member) \
	container_of(list, type, member)

#define LIST_APPEND(entry, list) \
	do { \
		(list)->next->prev = (entry); \
		(entry)->next = (list)->next; \
		(entry)->prev = (list); \
		(list)->next = (entry); \
	} while(0)

#define LIST_DELETE(entry) \
	do { \
		(entry)->prev->next = (entry)->next; \
		(entry)->next->prev = (entry)->prev; \
		(entry)->prev = (entry); \
		(entry)->next = (entry); \
	} while(0)

inline size_t LIST_LENGTH(struct list_head* list) {
	struct list_head* cursor;
	size_t len = 0;

	LIST_FOR_EACH(cursor, list) {
		len++;
	}
	return len;
}

#endif
