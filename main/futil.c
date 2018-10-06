#include <string.h>

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

char* futil_get_fext(char* path) {
	size_t len = strlen(path);
	char* fext_ptr = path + len;
	while(fext_ptr-- > path) {
		if(*fext_ptr == '.') {
			return fext_ptr + 1;
		}
	}
	return NULL;
}
