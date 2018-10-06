#ifndef _HTTPD_H_
#define _HTTPD_H_

#include <http_server.h>

#include "esp_err.h"

#include "list.h"

#ifndef HTTPD_302
#define HTTPD_302 "302 Found"
#endif

struct httpd {
	httpd_handle_t server;
	char* webroot;

	struct list_head static_file_handlers;
};

struct httpd_handler;

struct httpd_handler_ops {
	void (*free)(struct httpd_handler* hndlr);
};

struct httpd_handler {
	struct list_head list;
	httpd_uri_t uri_handler;
	struct httpd_handler_ops* ops;
};

struct httpd_static_file_handler {
	struct httpd_handler handler;
	char* path;
};

struct httpd_redirect_handler {
	struct httpd_handler handler;
	char* location;	
};

esp_err_t httpd_alloc(struct httpd** retval, const char* webroot);
esp_err_t __httpd_add_static_path(struct httpd* httpd, char* dir, char* name);
esp_err_t httpd_add_redirect(struct httpd* httpd, char* from, char* to);

#define httpd_add_static_path(httpd, path) \
	__httpd_add_static_path(httpd, NULL, path)

#endif
