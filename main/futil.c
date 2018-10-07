#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "futil.h"

void futil_normalize_path(char* path) {
	size_t len = strlen(path);
	char* slash_ptr = path, *end_ptr = path + len;
	while(slash_ptr < end_ptr && (slash_ptr = strchr(slash_ptr, '/'))) {
		while(++slash_ptr < end_ptr && *slash_ptr == '/') {
			memmove(slash_ptr, slash_ptr + 1, end_ptr - slash_ptr);
			slash_ptr--;
		}
	}
}


esp_err_t futil_relpath(char* path, char* basepath) {
	size_t base_len = strlen(basepath);
	size_t path_len = strlen(path);
	
	if(base_len > path_len) {
		return ESP_ERR_INVALID_ARG;
	}

	if(strncmp(basepath, path, base_len)) {
		return ESP_ERR_INVALID_ARG;
	}

	memmove(path, path + base_len, path_len - base_len + 1);

	return ESP_OK;
}

static char* futil_get_fext_limit(char* path, char* limit) {
	if(limit < path) {
		return NULL;
	}
	char* fext_ptr = limit;
	while(fext_ptr-- > path) {
		if(*fext_ptr == '.') {
			return fext_ptr + 1;
		}
	}
	return NULL;
}

char* futil_get_fext(char* path) {
	return futil_get_fext_limit(path, path + strlen(path));
}

esp_err_t futil_get_bytes(void* dst, size_t len, char* path) {
	esp_err_t err = ESP_OK;
	int fd = open(path, O_RDONLY);
	if(fd < 0) {
		err = errno;
		goto fail;
	}

	while(len > 0) {
		ssize_t read_len = read(fd, dst, len);
		if(read_len < 0) {
			err = errno;
			goto fail_fd;
		}

		if(read_len == 0) {
			err = ESP_ERR_INVALID_ARG;
			goto fail_fd;
		}

		dst += read_len;
		len -= read_len;
	}

fail_fd:
	close(fd);
fail:
	return err;
}
