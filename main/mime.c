#include <string.h>

#include "mime.h"
#include "futil.h"
#include "util.h"

const struct mime_pair mimedb[] = {
	{
		.fext = "jpg",
		.mime_type = "image/jpeg",
	},
	{
		.fext = "jpeg",
		.mime_type = "image/jpeg",
	},
	{
		.fext = "png",
		.mime_type = "image/png",
	},
};

const char* mime_get_type_from_filename(char* path) {
	size_t i;
	const char* fext = futil_get_fext(path);
	if(!fext) {
		return NULL;
	}

	for(i = 0; i < ARRAY_LEN(mimedb); i++) {
		const struct mime_pair* mime = &mimedb[i];
		if(!strcmp(mime->fext, fext)) {
			return mime->mime_type;
		}
	}
	return NULL;
}