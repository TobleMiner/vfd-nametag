#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#include "esp_err.h"

#include "httpd.h"
#include "util.h"
#include "ip.h"

static void httpd_normalize_path(char* path) {
	size_t len = strlen(path);
	char* slash_ptr = path, *end_ptr = path + len;
	while(slash_ptr < end_ptr && (slash_ptr = strchr(slash_ptr, '/'))) {
		while(++slash_ptr < end_ptr && *slash_ptr == '/') {
			memmove(slash_ptr, slash_ptr + 1, end_ptr - slash_ptr);
			slash_ptr--;
		}
	}
}

esp_err_t httpd_alloc(struct httpd** retval, const char* webroot) {
	esp_err_t err;
	struct httpd* httpd;
	httpd_config_t conf = HTTPD_DEFAULT_CONFIG();

	if(!ip_stack_initialized()) {
		err = ESP_ERR_INVALID_STATE;
		goto fail;
	}

	httpd = calloc(1, sizeof(struct httpd));
	if(!httpd) {
		err = ESP_ERR_NO_MEM;
		goto fail;
	}

	httpd->webroot = strdup(webroot);
	if(!httpd->webroot) {
		err = ESP_ERR_NO_MEM;
		goto fail_alloc;
	}

	httpd_normalize_path(httpd->webroot);

	INIT_LIST_HEAD(httpd->static_file_handlers);

	if((err = httpd_start(&httpd->server, &conf))) {
		goto fail_webroot_alloc;
	}

	*retval = httpd;
	return ESP_OK;

fail_webroot_alloc:
	free(httpd->webroot);
fail_alloc:
	free(httpd);
fail:
	return err;
}

static esp_err_t xlate_err(int err) {
	switch(err) {
		case ENOMEM:
			return ESP_ERR_NO_MEM;
		case EBADF:
		case EACCES:
		case ENOENT:
		case ENOTDIR:
		case EIO:
		case ENAMETOOLONG:
		case EOVERFLOW:
			return ESP_ERR_INVALID_ARG;
		case EMFILE:
		case ENFILE:
		case ELOOP:
			return ESP_ERR_INVALID_STATE;
	}
	return ESP_FAIL;
}

static esp_err_t static_file_get_handler(httpd_req_t* req) {
	int fd;
	ssize_t read_len;
	char buff[256];
	esp_err_t err = ESP_OK;
	struct httpd_static_file_handler* hndlr = req->user_ctx;

	printf("httpd: Delivering static content from %s\n", hndlr->path);

	if((fd = open(hndlr->path, O_RDONLY)) < 0) {
		err = xlate_err(errno);
		goto fail;
	}

	while((read_len = read(fd, buff, sizeof(buff))) > 0) {
		if((err = httpd_resp_send_chunk(req, buff, read_len))) {
			printf("Failed to send chunk: %d size: %zd\n", err, read_len);
			goto fail_open;
		}
	}

	if(read_len < 0) {
		printf("Read failed: %s(%d)\n", strerror(errno), errno);
		err = xlate_err(errno);
		goto fail_open;
	}

	httpd_resp_send_chunk(req, NULL, 0);

fail_open:
	close(fd);
fail:
	return err;
}

// Use with normalized paths only!
static esp_err_t httpd_relpath(char* path, char* basepath) {
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

#define HTTPD_HANDLER_TO_HTTPD_STATIC_FILE_HANDER(hndlr) \
	container_of((hndlr), struct httpd_static_file_handler, handler)

static void httpd_free_static_file_handler(struct httpd_handler* hndlr) {
	struct httpd_static_file_handler* hndlr_static_file = HTTPD_HANDLER_TO_HTTPD_STATIC_FILE_HANDER(hndlr);
	free(hndlr_static_file->path);
	// Allthough uri_handler.uri is declared const we use it with dynamically allocated memory
	free((char*)hndlr->uri_handler.uri);
}

struct httpd_handler_ops httpd_static_file_handler_ops = {
	.free = httpd_free_static_file_handler,
};

static esp_err_t httpd_add_static_file(struct httpd* httpd, char* path) {
	esp_err_t err;
	char* uri;
	struct httpd_static_file_handler* hndlr = calloc(1, sizeof(struct httpd_static_file_handler));
	if(!hndlr) {
		err = ESP_ERR_NO_MEM;
		goto fail;
	}

	hndlr->path = strdup(path);
	if(!hndlr->path) {
		err = ESP_ERR_NO_MEM;
		goto fail_alloc;
	}

	uri = strdup(path);
	if(!uri) {
		err = ESP_ERR_NO_MEM;
		goto fail_path_alloc;
	}

	httpd_normalize_path(uri);
	if((err = httpd_relpath(uri, httpd->webroot))) {
		goto fail_uri_alloc;
	}

	hndlr->handler.uri_handler.uri = uri;
	hndlr->handler.uri_handler.method = HTTP_GET;
	hndlr->handler.uri_handler.handler = static_file_get_handler;
	hndlr->handler.uri_handler.user_ctx = hndlr;

	hndlr->handler.ops = &httpd_static_file_handler_ops;

	printf("httpd: Registering static file handler at '%s' for file '%s'\n", uri, path);

	if((err = httpd_register_uri_handler(httpd->server, &hndlr->handler.uri_handler))) {
		goto fail_uri_alloc;
	}

	LIST_APPEND(&hndlr->handler.list, &httpd->static_file_handlers);

	return ESP_OK;

fail_uri_alloc:
	free(uri);
fail_path_alloc:
	free(hndlr->path);
fail_alloc:
	free(hndlr);
fail:
	return err;
}


#define HTTPD_HANDLER_TO_HTTPD_REDIRECT_HANDER(hndlr) \
	container_of((hndlr), struct httpd_redirect_handler, handler)

static void httpd_free_redirect_handler(struct httpd_handler* hndlr) {
	struct httpd_redirect_handler* hndlr_redirect = HTTPD_HANDLER_TO_HTTPD_REDIRECT_HANDER(hndlr);
	free(hndlr_redirect->location);
	// Allthough uri_handler.uri is declared const we use it with dynamically allocated memory
	free((char*)hndlr->uri_handler.uri);
}

struct httpd_handler_ops httpd_redirect_handler_ops = {
	.free = httpd_free_redirect_handler,
};

static esp_err_t redirect_handler(httpd_req_t* req) {
	esp_err_t err;
	struct httpd_redirect_handler* hndlr = req->user_ctx;

	if((err = httpd_resp_set_status(req, HTTPD_302))) {
		goto fail;
	}

	if((err = httpd_resp_set_hdr(req, "Location", hndlr->location))) {
		goto fail;
	}
	
	printf("httpd: Delivering redirect to %s\n", hndlr->location);

	httpd_resp_send_chunk(req, NULL, 0);

fail:
	return err;
}

esp_err_t httpd_add_redirect(struct httpd* httpd, char* from, char* to) {
	esp_err_t err;
	char* uri;
	struct httpd_redirect_handler* hndlr = calloc(1, sizeof(struct httpd_redirect_handler));
	if(!hndlr) {
		err = ESP_ERR_NO_MEM;
		goto fail;
	}

	hndlr->location = strdup(to);
	if(!hndlr->location) {
		err = ESP_ERR_NO_MEM;
		goto fail_alloc;
	}

	uri = strdup(from);
	if(!uri) {
		err = ESP_ERR_NO_MEM;
		goto fail_path_alloc;
	}

	hndlr->handler.uri_handler.uri = uri;
	hndlr->handler.uri_handler.method = HTTP_GET;
	hndlr->handler.uri_handler.handler = redirect_handler;
	hndlr->handler.uri_handler.user_ctx = hndlr;

	hndlr->handler.ops = &httpd_redirect_handler_ops;

	printf("httpd: Registering redirect handler at '%s' for location '%s'\n", from, to);

	if((err = httpd_register_uri_handler(httpd->server, &hndlr->handler.uri_handler))) {
		goto fail_uri_alloc;
	}

	LIST_APPEND(&hndlr->handler.list, &httpd->static_file_handlers);

	return ESP_OK;

fail_uri_alloc:
	free(uri);
fail_path_alloc:
	free(hndlr->location);
fail_alloc:
	free(hndlr);
fail:
	return err;
}

static esp_err_t httpd_add_static_directory(struct httpd* httpd, char* path) {
	esp_err_t err;
	struct dirent* cursor;
	DIR* dir = opendir(path);
	if(!dir) {
		return xlate_err(errno);
	}
	DIRENT_FOR_EACH(cursor, dir) {
		if(!cursor) {
			err = xlate_err(errno);
		}
		if((err = __httpd_add_static_path(httpd, path, cursor->d_name))) {
			goto fail;
		}
	}
	closedir(dir);
	return ESP_OK;
fail:
	closedir(dir);
	return err;
}

esp_err_t __httpd_add_static_path(struct httpd* httpd, char* dir, char* name) {
	int err;
	struct stat pathinfo;
	char* path = name;
	if(dir) {
		// $dir/$name\0
		size_t len = strlen(dir) + 1 + strlen(name) + 1;
		path = calloc(1, len);
		if(!path) {
			err = ESP_ERR_NO_MEM;
			goto fail;
		}
		strcat(path, dir);
		strcat(path, "/");
		strcat(path, name);
	}
	printf("Stating '%s'\n", path);
	if(stat(path, &pathinfo)) {
		printf("Stat failed: %s(%d)\n", strerror(errno), errno);
		err = xlate_err(errno);
		goto fail_alloc;
	}
	if(pathinfo.st_mode & S_IFDIR) {
		err = httpd_add_static_directory(httpd, path);
	} else if(pathinfo.st_mode & S_IFREG) {
		err = httpd_add_static_file(httpd, path);
	} else {
		err = ESP_ERR_INVALID_ARG;
	}

	if(err) {
		// TODO: Free all handlers
	}

fail_alloc:
	if(dir) {
		free(path);
	}
fail:
	return err;
}
