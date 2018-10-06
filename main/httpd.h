#ifndef _HTTPD_H_
#define _HTTPD_H_

#include <http_server.h>

#include "esp_err.h"

#include "list.h"

struct httpd {
	httpd_handle_t server;
	char* webroot;

	struct list_head static_file_handlers;
};

struct httpd_static_file_handler {
	struct list_head list;
	char* path;
	httpd_uri_t uri_handler;
};

esp_err_t httpd_alloc(struct httpd** retval, const char* webroot);
esp_err_t __httpd_add_static_path(struct httpd* httpd, char* dir, char* name);

#define httpd_add_static_path(httpd, path) \
	__httpd_add_static_path(httpd, NULL, path)

#endif
