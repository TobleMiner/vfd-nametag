#ifndef _FUTIL_H_
#define _FUTIL_H_

#include "esp_err.h"

void futil_normalize_path(char* path);
esp_err_t futil_relpath(char* path, char* basepath);
char* futil_get_fext(char* path);
esp_err_t futil_get_bytes(void* dst, size_t len, char* path);

#endif
