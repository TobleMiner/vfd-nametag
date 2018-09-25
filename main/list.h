#ifndef _LIST_H_
#define _LIST_H_

#include "util.h"

struct list_head;

struct list_head {
	struct list_head *next, *prev;
};

#define INIT_LIST_HEAD(name) \
	(name) = (struct list_head){ &(name), &(name) }

#define LIST_FOR_EACH(cursor, list) \
	for((cursor) = (list)->next; (cursor) != (list); (cursor) = (cursor)->next)

#define LIST_GET_ENTRY(list, type, member) \
	container_of(list, type, member)

#define LIST_APPEND(entry, list) \
	do { \
		(list)->next->prev = (entry); \
		(entry)->next = (list)->next; \
		(entry)->prev = (list); \
		(list)->next = (entry); \
	} while(0)

#endif
