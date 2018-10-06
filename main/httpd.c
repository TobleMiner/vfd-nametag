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

	httpd->webroot = webroot;

	INIT_LIST_HEAD(httpd->static_file_handlers);

	if((err = httpd_start(&httpd->server, &conf))) {
		goto fail_alloc;
	}

	*retval = httpd;
	return ESP_OK;

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
	if((fd = open(hndlr->path, O_RDONLY)) < 0) {
		err = xlate_err(errno);
		goto fail;
	}

	while((read_len = read(fd, buff, sizeof(buff))) > 0) {
		if((err = httpd_resp_send_chunk(req, buff, read_len))) {
			goto fail_open;
		}
	}

	if(read_len < 0) {
		err = xlate_err(errno);
		goto fail_open;
	}

	err = httpd_resp_send_chunk(req, NULL, 0);

fail_open:
	close(fd);
fail:
	return err;
}

static esp_err_t httpd_add_static_file(struct httpd* httpd, char* path) {
	esp_err_t err;
	struct httpd_static_file_handler* hndlr;
	printf("httpd: Adding static file from '%s'\n", path);
	hndlr = calloc(1, sizeof(hndlr));
	if(!hndlr) {
		err = ESP_ERR_NO_MEM;
		goto fail;
	}

	hndlr->path = strdup(path);
	if(!hndlr->path) {
		err = ESP_ERR_NO_MEM;
		goto fail_alloc;
	}

	hndlr->uri_handler.uri = hndlr->path;
	hndlr->uri_handler.method = HTTP_GET;
	hndlr->uri_handler.handler = static_file_get_handler;
	hndlr->uri_handler.user_ctx = hndlr;

	if((err = httpd_register_uri_handler(httpd->server, &hndlr->uri_handler))) {
		goto fail_path_alloc;
	}

	LIST_APPEND(&hndlr->list, &httpd->static_file_handlers);

	return ESP_OK;

fail_path_alloc:
	free(hndlr->path);
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
