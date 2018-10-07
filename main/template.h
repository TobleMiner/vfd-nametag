#ifndef _TEMPLATE_H_
#define _TEMPLATE_H_

#include "list.h"

#define TEMPLATE_ID_LEN_DEFAULT 16
#define TEMPLATE_ID_PREFIX "[/{"
#define TEMPLATE_ID_SUFFIX "}/]"

#define TEMPLATE_BUFF_SIZE 256

struct templ {
	struct list_head templates;
};

// priv comes from templ_entry struct, ctx from current execution
typedef (*templ_cb)(void* ctx, void* priv);

struct templ_entry {
	list_head list;

	char* id;
	void* priv;
	templ_cb cb;
};

struct templ_instance {
	struct list_head slices;
}:

struct templ_slice {
	list_head list;

	size_t start;
	size_t end;
	struct templ_entry* entry;
};

typedef (*templ_write_cb)(void* ctx, char* buff, size_t* len);

void template_init(struct templ* templ);
esp_err_t template_alloc_instance(struct templ_instance** retval, struct templ* templ, int fd);
esp_err_t template_add(struct templ* templ, char* id, templ_cb cb, void* priv);
esp_err_t template_apply(struct templ_instance* instance, int fd, templ_write_cb cb, void* ctx);

#endif
