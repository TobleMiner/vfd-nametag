#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "esp_err.h"

#include "template.h"
#include "ring.h"
#include "util.h"

void template_init(struct templ* templ) {
	INIT_LIST_HEAD(templ->templates);
}

static esp_err_t template_alloc_slice(struct templ_slice** retval) {
	struct templ_slice* slice = calloc(1, sizeof(struct templ_slice));
	if(!slice) {
		return ESP_ERR_NO_MEM;
	}

	*retval = slice;
	return ESP_OK;
}

void template_free_instance(struct templ_instance* instance) {
	struct list_head* cursor;

	LIST_FOR_EACH(cursor, &instance->slices) {
		struct templ_slice* slice = LIST_GET_ENTRY(cursor, struct templ_slice, list);
		free(slice);
	}
	free(instance);
}

esp_err_t template_alloc_instance(struct templ_instance** retval, struct templ* templ, char* path) {
	esp_err_t err;
	int fd = open(path, O_RDONLY);
	if(fd < 0) {
		err = errno;
		goto fail;
	}

	err = template_alloc_instance_fd(retval, templ, fd);

	close(fd);

fail:
	return err;
};

esp_err_t template_alloc_instance_fd(struct templ_instance** retval, struct templ* templ, int fd) {
	esp_err_t err;
	size_t max_id_len = TEMPLATE_ID_LEN_DEFAULT, filepos = 0;
	ssize_t read_len = 0;
	struct templ_slice* slice;
	struct ring* ring;
	struct list_head* cursor;
	struct templ_instance* instance = calloc(1, sizeof(struct templ_instance));
	if(!instance) {
		err = ESP_ERR_NO_MEM;
		goto fail;
	}

	INIT_LIST_HEAD(instance->slices);

	LIST_FOR_EACH(cursor, &templ->templates) {
		struct templ_entry* entry = LIST_GET_ENTRY(cursor, struct templ_entry, list);
		max_id_len = max(max_id_len, strlen(entry->id));
	}

	if((err = ring_alloc(&ring, max_id_len * 2))) {
		goto fail_instance_alloc;
	}

	if((err = template_alloc_slice(&slice))) {
		goto fail_ring_alloc;
	}
	LIST_APPEND(&slice->list, &instance->slices);

	do {
		read_len = read(fd, ring->ptr_write, ring_free_space_contig(ring));
		if(read_len < 0) {
			err = errno;
			goto fail_slices;
		}
		ring_advance_write(ring, read_len);

next:
		while(ring_available(ring) >= (read_len == 0 ? 0 : max_id_len)) {
			LIST_FOR_EACH(cursor, &templ->templates) {
				struct templ_slice* prev_slice;
				struct templ_entry* entry = LIST_GET_ENTRY(cursor, struct templ_entry, list);
				size_t id_len = strlen(entry->id);
				if(!ring_memcmp(ring, entry->id, id_len, NULL)) {
					slice->end = filepos;

					prev_slice = slice;
					if((err = template_alloc_slice(&slice))) {
						goto fail_slices;
					}
					slice->entry = entry;
					slice->start = prev_slice->end;
					slice->end = filepos + id_len;
					LIST_APPEND(&slice->list, &prev_slice->list);

					prev_slice = slice;
					if((err = template_alloc_slice(&slice))) {
						goto fail_slices;
					}
					slice->start = prev_slice->end;
					LIST_APPEND(&slice->list, &prev_slice->list);

					filepos += id_len;
					goto next;
				}
			}

			ring_inc_read(ring);
			filepos++;
		}
	} while(read_len);
	slice->end = filepos;
	ring_free(ring);

	*retval = instance;
	return ESP_OK;

fail_slices:
	free(slice);
	LIST_FOR_EACH(cursor, &instance->slices) {
		struct templ_slice* slice = LIST_GET_ENTRY(cursor, struct templ_slice, list);
		free(slice);
	}
fail_ring_alloc:
	ring_free(ring);
fail_instance_alloc:
	free(instance);
fail:
	return err;
}

esp_err_t template_add(struct templ* templ, char* id, templ_cb cb, void* priv) {
	esp_err_t err;
	size_t buff_len;
	struct templ_entry* entry = calloc(1, sizeof(struct templ_entry));
	if(!entry) {
		err = ESP_ERR_NO_MEM;
		goto fail;
	}

	entry->cb = cb;
	entry->priv = priv;

	buff_len = strlen(TEMPLATE_ID_PREFIX) + strlen(id) + strlen(TEMPLATE_ID_SUFFIX) + 1;
	entry->id = calloc(1, buff_len);
	if(!entry->id) {
		err = ESP_ERR_NO_MEM;
		goto fail_entry_alloc;
	}
	snprintf(entry->id, buff_len, "%s%s%s", TEMPLATE_ID_PREFIX, id, TEMPLATE_ID_SUFFIX);

	LIST_APPEND(&entry->list, &templ->templates);

fail_entry_alloc:
	free(entry);
fail:
	return err;
}

esp_err_t template_apply(struct templ_instance* instance, char* path, templ_write_cb cb, void* ctx) {
	esp_err_t err;
	int fd = open(path, O_RDONLY);
	if(fd < 0) {
		err = errno;
		goto fail;
	}

	err = template_apply_fd(instance, fd, cb, ctx);

	close(fd);

fail:
	return err;
};

esp_err_t template_apply_fd(struct templ_instance* instance, int fd, templ_write_cb cb, void* ctx) {
	esp_err_t err = ESP_OK;
	size_t filepos = 0;
	char buff[TEMPLATE_BUFF_SIZE];
	struct list_head* cursor;

	LIST_FOR_EACH(cursor, &instance->slices) {
		struct templ_slice* slice = LIST_GET_ENTRY(cursor, struct templ_slice, list);

		// Skip to start of slice
		while(filepos < slice->start) {
			size_t max_read_len = min(sizeof(buff), slice->start - filepos);
			ssize_t read_len = read(fd, buff, max_read_len);
			if(read_len < 0) {
				err = errno;
				goto fail;
			}
			if(read_len == 0) {
				err = ESP_ERR_INVALID_ARG;
				goto fail;
			}
			filepos += read_len;
		}


		if(slice->entry) {
			if((err = slice->entry->cb(ctx, slice->entry->priv))) {
				goto fail;
			}
		} else {
			// Write out slice
			while(filepos < slice->end) {
				size_t max_read_len = min(sizeof(buff), slice->end - filepos);
				ssize_t read_len = read(fd, buff, max_read_len);
				if(read_len < 0) {
					err = errno;
					goto fail;
				}
				if(read_len == 0) {
					err = ESP_ERR_INVALID_ARG;
					goto fail;
				}
				filepos += read_len;

				if((err = cb(ctx, buff, read_len))) {
					goto fail;
				}
			}
		}
	}

fail:
	return err;
}